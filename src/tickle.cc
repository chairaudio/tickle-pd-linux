/*
    © Copyright 2018, The Center for Haptic Audio Interaction Research
    Sebastian Stang, Max Neupert, Clemens Wegener

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. See the markup file gpl-3.0.md
    If not, see <http://www.gnu.org/licenses/>.

 */

#include <m_pd.h>

#include <cstring>
#include <chrono>
#include <thread>
using namespace std::chrono_literals;

extern "C" {
void tickle_tilde_setup(void);
}

#include "./device.h++"
using tickle::Device;

#include "./shared_device_manager.h++"
using tickle::SharedDeviceManager;
static SharedDeviceManager shared_device_manager;

#include "./client.h++"
using tickle::Client;
using tickle::ClientHandle;

#if __has_include("./gitversion.h")
#include "./gitversion.h"
#else
#define GITVERSION "NaN"
#endif

static t_class* tickle_tilde_class;

typedef struct _tickle {
    t_object x_obj;
    t_outlet* audio_out;
    t_outlet* data_out;

    t_clock* poll_clock;
    bool is_active;
    bool keep_polling;
    uint16_t latency;

    ClientHandle client;

} tickle_t;

/* -------------- forward decls ---------------- */

static t_int tickle_pollbang(tickle_t*);
static void info_method(tickle_t*);
static void tickle_toggle_polling(tickle_t*, float);

/* -------------- structors ---------------- */

static void* tickle_new(t_symbol* s, int argc, t_atom* argv) {
    tickle_t* self = (tickle_t*)pd_new(tickle_tilde_class);

    auto& x_obj = self->x_obj;
    self->audio_out = outlet_new(&x_obj, &s_signal);
    self->data_out = outlet_new(&x_obj, 0);

    self->poll_clock = clock_new(&x_obj, (t_method)tickle_pollbang);
    self->is_active = false;
    self->keep_polling = false;

    self->client = shared_device_manager.create_client();

    self->latency = 0;

    return (void*)self;
}

void tickle_delete(tickle_t* self) {
    shared_device_manager.dispose_client(self->client);
    tickle_toggle_polling(self, 0);
    std::this_thread::sleep_for(5ms);
    clock_free(self->poll_clock);
}

/* -------------- info methods ---------------- */

static void info_method(tickle_t* self) {
    if (auto device = self->client->get_device_handle()) {
        auto [serial, version] = (*device)->get_info().value();
        post("device serial %s", serial.c_str());
        post("device version %s", version.c_str());
    } else {
        error("no device available");
    }
}

static void print_external_info() {
    post("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    post("The Center for Haptic Audio Interaction Research");
    post("external version %s", GITVERSION);
    post("compiled on %s at %s for pd %d.%d.%d", __DATE__, __TIME__,
         PD_MAJOR_VERSION, PD_MINOR_VERSION, PD_BUGFIX_VERSION);
    post("kernel module version %s",
         shared_device_manager.get_kernel_module_version().c_str());
    post("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
}

/* -------------- sending things out ---------------- */

static void tickle_bang(tickle_t* self) {
    auto changes = self->client->compare_frames();

    if (changes.position) {
        auto position = changes.position.value();
        std::array<t_atom, 3> position_out;
        SETFLOAT(&position_out[0], 0);
        SETFLOAT(&position_out[1], position.x);
        SETFLOAT(&position_out[2], position.y);
        outlet_anything(self->data_out, gensym("position"), 3,
                        position_out.data());
    }

    for (auto change : changes.rotary) {
        if (not change) {
            continue;
        }
        auto rotary = change.value();
        std::array<t_atom, 2> rotary_out;
        SETFLOAT(&rotary_out[0], rotary.index);
        SETFLOAT(&rotary_out[1], rotary.value);
        outlet_anything(self->data_out, gensym("rotary"), 2, rotary_out.data());
    }

    for (auto change : changes.buttons) {
        if (not change) {
            continue;
        }
        auto button = change.value();
        std::array<t_atom, 2> button_out;
        SETSYMBOL(&button_out[0], button.index ? gensym("up") : gensym("down"));
        SETFLOAT(&button_out[1], button.value);
        outlet_anything(self->data_out, gensym("button"), 2, button_out.data());
    }

    auto ring_size = self->client->get_ring_size();
    if (self->latency != ring_size) {
        self->latency = ring_size;
        std::array<t_atom, 1> latency_out;
        SETFLOAT(&latency_out[0], self->latency);
        outlet_anything(self->data_out, gensym("latency"), 1,
                        latency_out.data());
    }
}

/* -------------- poll clock callback ---------------- */

static t_int tickle_pollbang(tickle_t* self) {
    tickle_bang(self);
    if (self->keep_polling) {
        clock_delay(self->poll_clock, 1);
    }
    return 1;
}

static void tickle_toggle_polling(tickle_t* self, float value) {
    if (value == 0) {
        self->keep_polling = self->is_active = false;
    } else {
        if (not self->client->get_device_handle()) {
            error("no device available");
            return;
        }

        self->keep_polling = true;
        if (not self->is_active) {
            self->is_active = true;
            clock_delay(self->poll_clock, 1000);
        }
    }
}

static t_int* tickle_dsp_perform_audio_out(t_int* w) {
    tickle_t* self = (tickle_t*)(w[1]);
    t_sample* out = (t_sample*)(w[2]);
    int n_samples = (int)(w[3]);

    self->client->fill_audio_buffer(out, n_samples);
    if (not self->is_active || not self->client->has_device()) {
        memset(out, 0, n_samples * sizeof(t_sample));
    }

    return (w + 4);
}

static void tickle_dsp_setup(tickle_t* self, t_signal** sp) {
    /*
    if (not self->client->get_device_handle()) {
        error("no device available");
        return;
    }
    if (not self->is_active) {
        error("device is not active");
        return;
    } */

    if (sp[0]->s_sr != 48000) {
        error("tickle~ is best served at 48khz");
    }
    self->client->prepare_dsp(sp[0]->s_sr, sp[0]->s_vecsize);
    dsp_add(tickle_dsp_perform_audio_out, 3, self, sp[0]->s_vec, sp[0]->s_n);
}

void tickle_dim(tickle_t* self, float value) {
    shared_device_manager.dim(value);
}

void tickle_led(tickle_t* self, float index, float r, float g, float b) {
    uint32_t color = b * 255;
    color <<= 8;
    color += int(r * 255);
    color <<= 8;
    color += int(g * 255);
    shared_device_manager.set_color(index, color);
}

void tickle_tilde_setup() {
    tickle_tilde_class = reinterpret_cast<t_class*>(class_new(
        gensym("tickle~"), (t_newmethod)tickle_new, (t_method)tickle_delete,
        sizeof(tickle_t), (t_atomtype)CLASS_DEFAULT, A_GIMME, (t_atomtype)0));

    class_addbang(tickle_tilde_class, tickle_bang);
    class_addfloat(tickle_tilde_class, (t_method)tickle_toggle_polling);
    class_addmethod(tickle_tilde_class, (t_method)tickle_dsp_setup,
                    gensym("dsp"), A_CANT, (t_atomtype)0);
    class_addmethod(tickle_tilde_class, (t_method)info_method, gensym("info"),
                    (t_atomtype)0);
    class_addmethod(tickle_tilde_class, (t_method)tickle_dim, gensym("dim"),
                    A_FLOAT, (t_atomtype)0);
    class_addmethod(tickle_tilde_class, (t_method)tickle_led, gensym("led"),
                    A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, (t_atomtype)0);
    std::this_thread::sleep_for(1s);

    print_external_info();
}
