/* Portable test host for the real pinned EMU8000 C source.
   Only the hardware inputs and 86Box host services are synthetic. */
#define _GNU_SOURCE
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <86box/sound.h>
#include <86box/snd_emu8k.h>
#include "emu8k_test_effect_registers.h"

enum { native_rate = 44100, ram_kib = 28672, port = 0x620 };
enum { filter_bypass, filter_static, filter_wet, filter_stepped,
       filter_lfo, filter_attack, filter_ports };

int wavetable_pos_global;
static unsigned init_fault, opens, live_allocations, allocation_calls, handlers;
static bool short_rom, fail_cache;

uint16_t emu8k_inw(uint16_t address, void *chip);
void emu8k_outw(uint16_t address, uint16_t value, void *chip);
void *__real_calloc(size_t count, size_t size);
void __real_free(void *pointer);
int __real_fclose(FILE *file);

void *__wrap_calloc(size_t count, size_t size) {
    ++allocation_calls;
    if (fail_cache && count == ((1u << 21) + 1u) && size == sizeof(int32_t))
        return NULL;
    void *p = __real_calloc(count, size);
    live_allocations += p != NULL;
    return p;
}

void __wrap_free(void *p) {
    live_allocations -= p != NULL;
    __real_free(p);
}

int __wrap_fclose(FILE *file) {
    --opens;
    return __real_fclose(file);
}

int AWE64_AuditEmuInitFailure(void) { return (int)init_fault; }
void fatal(const char *format, ...) {
    fputs(format, stderr);
    abort();
}

FILE *rom_fopen(const char *path, char *mode) {
    (void)path;
    (void)mode;
    FILE *file = tmpfile();
    if (!file) abort();
    int16_t rom[100];
    for (unsigned i = 0; i < 100; ++i)
        rom[i] = (int16_t)lrint(11000.0 * sin(2.0 * M_PI * i / 100.0));
    size_t left = 1048576u - (unsigned)short_rom;
    while (left) {
        const size_t count = left < sizeof(rom) ? left : sizeof(rom);
        if (fwrite(rom, 1, count, file) != count) abort();
        left -= count;
    }
    rewind(file);
    ++opens;
    return file;
}

#define IO_ARGS uint16_t base, uint16_t size, uint8_t (*inb)(uint16_t,void*), \
    uint16_t (*inw)(uint16_t,void*), uint32_t (*inl)(uint16_t,void*), \
    void (*outb)(uint16_t,uint8_t,void*), void (*outw)(uint16_t,uint16_t,void*), \
    void (*outl)(uint16_t,uint32_t,void*), void *context
void io_sethandler(IO_ARGS) { ++handlers; }
void io_removehandler(IO_ARGS) {}

static void release_chip(emu8k_t *chip) {
    // Mirrors the C++ host's separate empty allocation cleanup.
    emu8k_close(chip);
    __wrap_free(chip->empty);
    __wrap_free(chip);
}

static emu8k_t *new_chip(void) {
    emu8k_t *chip = __wrap_calloc(1, sizeof(*chip));
    if (!chip) abort();
    emu8k_init(chip, port, ram_kib);
    return chip;
}

static bool valid_chip(const emu8k_t *chip) {
    return chip->rom && chip->ram && chip->empty;
}

static void configure_voices(emu8k_t *chip, unsigned voices, int mode) {
    chip->hwcf3 = 4;
    for (unsigned i = 0; i < voices; ++i) {
        emu8k_voice_t *voice = &chip->voice[i];
        voice->addr.int_address = 2 + i;
        voice->loop_start.int_address = 2;
        voice->loop_end.int_address = 402;
        voice->cvcf_curr_volume = voice->volumeslide.last = voice->vtft_vol_target = 41000;
        voice->cpf_curr_pitch = voice->ptrx_pit_target = 0x4000;
        voice->cvcf_curr_filt_ctoff = voice->vtft_filter_target = 0xffff;
        voice->ifatn_init_filter = mode == filter_bypass ? 255 : 142;
        voice->filterq_idx = mode == filter_bypass ? 0 : 5;
        voice->filt_att = 65536;
        voice->vol_l = voice->vol_r = 170;
        voice->ptrx_revb_send = mode >= filter_wet ? 87 : 0;
        voice->csl_chor_send = mode >= filter_wet ? 51 : 0;
        voice->overshoot_samples = -1;
        if (mode >= filter_lfo) {
            voice->env_engine_on = 1;
            voice->ip = 0xe000;
            voice->vol_envelope.state = 5;
            voice->vol_envelope.value_db_oct = 180000;
            voice->mod_envelope.state = mode == filter_attack ? 2 : 5;
            voice->mod_envelope.attack_amount_amp_hz = 127 + i;
            voice->fmmod_lfo1_filt_mod = 18;
            voice->fixed_lfo1_filt_mod = 100;
            voice->pefe_modenv_filter_height = 60;
            voice->fixed_modenv_filter_height = 100;
            voice->lfo1_speed = 3ull << 32;
        }
    }
}

static void select_effects(emu8k_t *chip, int reverb, int bass, int treble) {
    uint16_t *registers[] = {NULL, chip->init1, chip->init2, chip->init3, chip->init4};
    for (unsigned i = 0; i < 28; ++i)
        registers[emu8k_rv_slots[i][0]][emu8k_rv_slots[i][1]] = emu8k_rv_parm[reverb][i];
    chip->init4[0x01] = emu8k_eq_bass_parm[bass][0];
    chip->init4[0x11] = emu8k_eq_bass_parm[bass][1];
    const unsigned section[] = {3, 3, 3, 4, 4, 4, 4, 4};
    const unsigned offset[] = {0x11, 0x13, 0x1b, 7, 0xb, 0xd, 0x17, 0x19};
    for (unsigned i = 0; i < 8; ++i)
        registers[section[i]][offset[i]] = emu8k_eq_treble_parm[treble][i];
}

static double cpu_milliseconds(void) {
    struct timespec value;
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &value)) abort();
    return value.tv_sec * 1000.0 + value.tv_nsec / 1.0e6;
}

static int render(emu8k_t *chip, unsigned voices, int mode, unsigned chunk,
                  unsigned frames, int effect, unsigned write_shift, int32_t *output) {
    uint32_t readback = 0;
    for (unsigned frame = 0; frame < frames;) {
        if (mode == filter_stepped) {
            for (unsigned i = 0; i < voices; ++i)
                chip->voice[i].filt_oct_curr = chip->voice[i].filt_oct_target =
                    (double)((frame + i) % 4000) / 4000.0;
        }
        const unsigned period = 4410;
        if (mode == filter_attack && frame % period == 0) {
            for (unsigned i = 0; i < voices; ++i) {
                chip->voice[i].mod_envelope.state = 2;
                chip->voice[i].mod_envelope.value_amp_hz = 0;
                chip->voice[i].overshoot_samples = 0;
            }
        }
        if (mode == filter_ports && frame % period == write_shift) {
            // Real IFATN writes and reads at a fixed emulated frame boundary.
            for (unsigned i = 0; i < voices; ++i) {
                const uint16_t value = (uint16_t)((90 + (frame / period * 13) % 122) << 8);
                emu8k_outw(port + 0x802, (uint16_t)(0x20 | i), chip);
                emu8k_outw(port + 0x800, value, chip);
                const uint16_t observed = emu8k_inw(port + 0x800, chip);
                if (observed != value) return 1;
                readback = readback * 33u + observed;
            }
        }
        if (effect >= 0 && frame == native_rate) {
            if (chip->rv_preset != effect || chip->eq_bass != 9 || chip->eq_treble != 2)
                return 2;
            select_effects(chip, (effect + 1) % 8, 1, 10);
        }
        unsigned count = frames - frame < chunk ? frames - frame : chunk;
        if (mode == filter_attack || mode == filter_ports || effect >= 0) {
            const unsigned until = period - (frame + period - write_shift) % period;
            if (count > until) count = until;
        }
        wavetable_pos_global = (int)count;
        emu8k_update(chip);
        memcpy(output + 2 * frame, chip->buffer, count * 2 * sizeof(*output));
        emu8k_reset_buffer(chip);
        wavetable_pos_global = 0;
        frame += count;
    }
    if (effect >= 0 && (chip->rv_preset != (effect + 1) % 8 ||
                        chip->eq_bass != 1 || chip->eq_treble != 10))
        return 2;
    if (mode == filter_ports && !readback) return 3;
    return 0;
}

static int check_lifetime(void) {
    for (unsigned i = 1; i <= 6; ++i) {
        init_fault = i == 6 ? 0 : i;
        short_rom = i == 6;
        handlers = 0;
        emu8k_t *chip = new_chip();
        if (chip->rom || chip->ram || chip->empty || handlers || opens ||
            live_allocations != 1) return 5;
        __wrap_free(chip);
    }
    init_fault = 0;
    short_rom = false;
    for (unsigned cycle = 0; cycle < 16; ++cycle) {
        emu8k_t *a = new_chip();
        emu8k_t *b = new_chip();
        if (!valid_chip(a) || !valid_chip(b) || opens) return 6;
#ifdef AWE64_CANDIDATE
        if (!a->mod_attack_cache || !b->mod_attack_cache ||
            a->mod_attack_cache == b->mod_attack_cache) return 7;
#endif
        configure_voices(a, 8, filter_attack);
        configure_voices(b, 8, filter_attack);
        int32_t output[128], control[128];
        const unsigned before = allocation_calls;
        if (render(a, 8, filter_attack, 44, 64, -1, 0, output)) return 8;
        if (render(b, 8, filter_attack, 44, 64, -1, 0, control) ||
            memcmp(output, control, sizeof(output))) return 8;
        release_chip(a);
        if (render(b, 8, filter_lfo, 44, 64, -1, 0, control) ||
            allocation_calls != before) return 8;
        release_chip(b);
        if (live_allocations || opens) return 9;
    }
    puts("LIFETIME_PASS five injection cases + real short ROM, 16 dual-chip cycles");
    return 0;
}

/* Same-pin differential fixture: near misses and fixed points, full voice
   bytes and raw PCM, including resumption after a silent interval. */
#ifdef AWE64_CANDIDATE
static int idle_state(const char *path) {
    emu8k_t *chip = new_chip();
    if (!valid_chip(chip)) return 20;
    memset(chip->voice, 0, sizeof(chip->voice));
    chip->hwcf3 = 4;
    for (unsigned i = 0; i < 32; ++i) {
        emu8k_voice_t *v = &chip->voice[i];
        v->addr.int_address = 10;
        v->addr.fract_address = 123;
        v->loop_start.int_address = 5;
        v->loop_end.int_address = 20;
        v->vtft_filter_target = 100 + i;
        v->filt_oct_target = i & 1 ? -0.0 : -0.25;
        v->vol_l = v->vol_r = 170;
        v->overshoot_samples = -1;
        v->ifatn_init_filter = 255;
    }
    chip->voice[1].vtft_vol_target = 20000;
    chip->voice[2].volumeslide.last = 20000;
    chip->voice[3].cvcf_curr_volume = 20000;
    chip->voice[4].ptrx_pit_target = 0x4000;
    chip->voice[5].cpf_curr_pitch = 0x4000;
    chip->voice[6].env_engine_on = 1;
    chip->voice[6].ip = 0xe000;
    chip->voice[6].vol_envelope.state = 5;
    chip->voice[6].vol_envelope.value_db_oct = 180000;
    chip->voice[7].addr.int_address = 34;
    chip->voice[8].loop_start.int_address = 10;
    chip->voice[8].loop_end.int_address = 10;
    chip->voice[9].addr.int_address = 0x01000010;
    chip->voice[9].loop_start.int_address = 10;
    chip->voice[9].loop_end.int_address = 10;
    chip->voice[10].ccca_qcontrol = 4;
    FILE *f = fopen(path, "wb");
    if (!f) return 21;
    ++opens;
    const unsigned chunks[] = {1, 44, 512, 3, 128, 1, 44};
    for (unsigned n = 0; n < sizeof(chunks)/sizeof(chunks[0]); ++n) {
        if (n == 3) configure_voices(chip, 12, filter_wet);
        if (n == 5) {
            /* Release: zero target is not zero current volume. */
            chip->voice[0].vtft_vol_target = 0;
            chip->voice[0].env_engine_on = 0;
        }
        wavetable_pos_global = (int)chunks[n];
        emu8k_update(chip);
        if (fwrite(chip->voice, sizeof(chip->voice), 1, f) != 1 ||
            fwrite(chip->buffer, 2 * sizeof(int32_t), chunks[n], f) != chunks[n]) return 22;
        emu8k_reset_buffer(chip);
        wavetable_pos_global = 0;
    }
    if (fclose(f)) return 23;
    release_chip(chip);
    return live_allocations || opens ? 24 : 0;
}
#endif

int main(int argc, char **argv) {
#ifdef AWE64_CANDIDATE
    if (argc == 3 && !strcmp(argv[1], "idle-state")) return idle_state(argv[2]);
#endif
    if (argc == 2 && !strcmp(argv[1], "lifetime")) return check_lifetime();
    if (argc != 9) return 64;
    const unsigned voices = (unsigned)atoi(argv[1]);
    const int mode = atoi(argv[2]), effect = atoi(argv[5]);
    const unsigned chunk = (unsigned)atoi(argv[3]), seconds = (unsigned)atoi(argv[4]);
    fail_cache = atoi(argv[6]) != 0;
    const unsigned write_shift = (unsigned)atoi(argv[7]);
    if (voices > 32 || mode < 0 || mode > filter_ports || effect < -1 || effect > 7 ||
        chunk < 1 || chunk > WTBUFLEN || seconds < 2 || seconds > 60 || write_shift > 1)
        return 64;

    emu8k_t *chip = new_chip();
    if (!valid_chip(chip) || opens) return 5;
#ifdef AWE64_CANDIDATE
    if ((chip->mod_attack_cache == NULL) != fail_cache) return 6;
#endif
    configure_voices(chip, voices, mode);
    if (effect >= 0) {
        select_effects(chip, effect, 9, 2);
        chip->chorus_engine.delay_samples_central = 750;
        chip->chorus_engine.delay_offset_samples_right = 123.625;
        chip->chorus_engine.lfodepth_multip = 101.;
        chip->chorus_engine.feedback = 128;
        chip->chorus_engine.lfo_inc.addr = 17ull << 32;
    }
    const unsigned frames = native_rate * seconds;
    int32_t *output = malloc((size_t)frames * 2 * sizeof(*output));
    if (!output) return 10;
    const unsigned before = allocation_calls;
    const double start = cpu_milliseconds();
    const int result = render(chip, voices, mode, chunk, frames, effect, write_shift, output);
    const double elapsed = cpu_milliseconds() - start;
    if (result || allocation_calls != before) return 11;
    unsigned nonzero = 0;
    for (unsigned i = 0; i < frames * 2; ++i) nonzero += output[i] != 0;
    if ((voices == 0) != (nonzero == 0)) return 12;
    if (strcmp(argv[8], "-")) {
        FILE *out = fopen(argv[8], "wb");
        if (!out) return 13;
        ++opens;
        const size_t written = fwrite(output, 2 * sizeof(*output), frames, out);
        if (fclose(out) || written != frames)
            return 13;
    }
    printf("cpu_ms=%.3f nonzero=%u\n", elapsed, nonzero);
    release_chip(chip);
    __real_free(output);
    return live_allocations || opens ? 14 : 0;
}
