#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "neske.h"

static void apu_write_catchup(struct nrom *nrom, enum apu_reg reg, uint8_t val)
{
    nrom->apu_mux.lock(nrom->apu_mux.mux);
    apu_reg_write(&nrom->apu, reg, val);
    nrom->apu_mux.unlock(nrom->apu_mux.mux);
}

static void _nrom_mem_write(void *mem, uint16_t addr, uint8_t data)
{
    struct nrom *nrom = mem;

    if (addr >= 0x2000 && addr < 0x4000)
    {
        addr = 0x2000 + addr % 8;
    }

    switch (addr)
    {
        case 0x2000: ppu_write(&nrom->ppu, PPUIO_CTRL, data); break;
        case 0x2001: ppu_write(&nrom->ppu, PPUIO_MASK, data); break;
        case 0x2003: ppu_write(&nrom->ppu, PPUIO_OAMADDR, data); break;
        case 0x2004: ppu_write(&nrom->ppu, PPUIO_OAMDATA, data); break;
        case 0x2005: ppu_write(&nrom->ppu, PPUIO_SCROLL, data); break;
        case 0x2006: ppu_write(&nrom->ppu, PPUIO_ADDR, data); break;
        case 0x2007: ppu_write(&nrom->ppu, PPUIO_DATA, data); break;

        case 0x4000: apu_write_catchup(nrom, APU_PULSE1_DDLC_NNNN, data); break; // pulse 1
        case 0x4001: apu_write_catchup(nrom, APU_PULSE1_EPPP_NSSS, data); break;
        case 0x4002: apu_write_catchup(nrom, APU_PULSE1_LLLL_LLLL, data); break;
        case 0x4003: apu_write_catchup(nrom, APU_PULSE1_LLLL_LHHH, data); break; 
        case 0x4004: apu_write_catchup(nrom, APU_PULSE2_DDLC_NNNN, data); break; // pulse 2
        case 0x4005: apu_write_catchup(nrom, APU_PULSE2_EPPP_NSSS, data); break;
        case 0x4006: apu_write_catchup(nrom, APU_PULSE2_LLLL_LLLL, data); break;
        case 0x4007: apu_write_catchup(nrom, APU_PULSE2_LLLL_LHHH, data); break;
        case 0x4008: apu_write_catchup(nrom, APU_TRIANG_CRRR_RRRR, data); break; // triangle
        case 0x400A: apu_write_catchup(nrom, APU_TRIANG_LLLL_LLLL, data); break;
        case 0x400B: apu_write_catchup(nrom, APU_TRIANG_LLLL_LHHH, data); break;
        case 0x400C: apu_write_catchup(nrom, APU_NOISER_XXLC_VVVV, data); break; // noise
        case 0x400E: apu_write_catchup(nrom, APU_NOISER_MXXX_PPPP, data); break;
        case 0x400F: apu_write_catchup(nrom, APU_NOISER_LLLL_LXXX, data); break;
        case 0x4015: apu_write_catchup(nrom, APU_STATUS_IFXD_NT21, data); break; // status
        case 0x4017: apu_write_catchup(nrom, APU_STATUS_MIXX_XXXX, data); break; // misc

        case 0x4014: // OAMDMA
            ppu_write_oam(&nrom->ppu, nrom->memory + (((uint16_t)data)<<8));
            nrom->cpu.cycles += nrom->cpu.cycles&2 + 513;
            break;
        case 0x4016:
            if (data&1)
            {
                nrom->controller_strobe = 0;
            }
            break;
        default:
            if (addr >= 0x8000) {
                assert(false);
            }
            if (nrom->prgsize == 1 && addr >= 0xC000) addr -= 0x4000;
            nrom->memory[addr] = data;
            break;
    }
}

void nrom_update_controller(struct nrom *nrom, struct controller_state cs)
{
    nrom->controller = cs;
}

static uint8_t _nrom_mem_read(void *mem, uint16_t addr)
{
    struct nrom *nrom = mem;
 
    if (addr >= 0x2000 && addr < 0x4000)
    {
        addr = 0x2000  +((addr-0x2000)%8);
    }

    uint8_t val = 0;

    switch (addr)
    {
        case 0x2002: return ppu_read(&nrom->ppu, PPUIO_STATUS);
        case 0x2004: return ppu_read(&nrom->ppu, PPUIO_OAMDATA);
        case 0x2007: return ppu_read(&nrom->ppu, PPUIO_DATA);
        case 0x4015:
            nrom->apu_mux.lock(nrom->apu_mux.mux);
            val = apu_reg_read(&nrom->apu, APU_STATUS_IFXD_NT21);
            nrom->apu_mux.unlock(nrom->apu_mux.mux);
            return val;
            break;
        case 0x4017:
            return 0; // controller, not apu, confusing ya
        case 0x4016:

            if (nrom->controller_strobe != 8)
            {
                return nrom->controller.btns[nrom->controller_strobe++];
            }
            else
            {
                return 1;
            }

            break;
        default: 
            if (nrom->prgsize == 1 && addr >= 0xC000) addr -= 0x4000;
            return nrom->memory[addr];
    }

    return 0;
}

uint16_t nrom_get_vector(struct nrom *nrom, enum vector vec)
{
    switch (vec)
    {
        case VEC_NMI: return   _nrom_mem_read(nrom, 0xFFFA) | (_nrom_mem_read(nrom, 0xFFFB) << 8);
        case VEC_RESET: return _nrom_mem_read(nrom, 0xFFFC) | (_nrom_mem_read(nrom, 0xFFFD) << 8);
        case VEC_IRQ: return   _nrom_mem_read(nrom, 0xFFFE) | (_nrom_mem_read(nrom, 0xFFFF) << 8);
    }

    return 0;
}


void nrom_reset(struct nrom *nrom)
{
    nrom->cpu = (struct ricoh_state){ 0 };
    nrom->cpu.pc = nrom_get_vector(nrom, VEC_RESET);
    nrom->cpu.flags = 0x24;
    nrom->cpu.sp = 0xFD;
    nrom->cpu.cycles = 7;
    nrom->apu = (struct apu){ 0 };
    apu_init(&nrom->apu);
}


uint8_t nrom_load(uint8_t *ines, struct nrom *out)
{
    struct mux_api mux = out->apu_mux;

    if (!(ines[0] == 'N' && ines[1] == 'E' && ines[2] == 'S' && ines[3] == 0x1A))
    {
        return 1;
    }

    uint32_t prg_size = ines[4]*(1<<14);

    if (prg_size > 0x8000) 
    {
        return 2;
    }

    uint32_t chr_size = ines[5]*(1<<13);

    if (chr_size > 0x2000)
    {
        return 3;
    }

    uint8_t mapper = (ines[6]>>4)|(ines[7]&0xF0);

    if (mapper != 0)
    {
        return 4;
    }
    
    mux.lock(mux.mux);
    *out = (struct nrom){ 0 };
    out->apu_mux = mux;
    out->decoder = make_ricoh_decoder();
    memcpy(out->memory+0x8000, ines+16, prg_size);
    out->prgsize = ines[4];
    out->chrsize = ines[5];
    out->controller_strobe = 8;
    nrom_reset(out);
    uint8_t mirroring = ines[6]&1;
    out->ppu = ppu_mk(mirroring ? PPUMIR_VER : PPUMIR_HOR);
    ppu_write_chr(&out->ppu, ines+16+prg_size, chr_size);
    mux.unlock(mux.mux);

    return 0;
}

struct ricoh_mem_interface nrom_get_memory_interface(struct nrom *nrom)
{
    return (struct ricoh_mem_interface){
        nrom,
        _nrom_mem_read,
        _nrom_mem_write,
    };
}

enum {
    DEV_CPU,
    DEV_PPU,
};

struct nrom_frame_result nrom_frame(struct nrom *nrom)
{
    struct ricoh_mem_interface mem = nrom_get_memory_interface(nrom);

    uint64_t cycles_start = nrom->cpu.cycles;

    while (!nrom->cpu.crash && (nrom->cpu.cycles-cycles_start) < 500000)
    {
        bool nmi_occured = false;

        int dev = DEV_CPU;
        uint64_t devc = nrom->cpu.cycles;

        if (devc*3 >= nrom->ppu.cycles) {
            devc = nrom->ppu.cycles/3;
            dev = DEV_PPU;
        }

        switch (dev)
        {
        case DEV_CPU:
            {
                struct instr_decoded decoded = ricoh_decode_instr(&nrom->decoder, &mem, nrom->cpu.pc);
                ricoh_run_instr(&nrom->cpu, decoded, &mem);
            }
            break;
        case DEV_PPU:
            {
                nmi_occured = ppu_cycle(&nrom->ppu, &mem);
            }
            break;
        }

        if (nmi_occured)
        {
            if (ppu_nmi_enabled(&nrom->ppu))
            {
                ricoh_do_interrupt(&nrom->cpu, &mem, nrom_get_vector(nrom, VEC_NMI));
            }
            break;
        }
    }

    struct nrom_frame_result result = { 0 };

    memcpy(result.screen, nrom->ppu.screen, sizeof result.screen);

    return result;
}
