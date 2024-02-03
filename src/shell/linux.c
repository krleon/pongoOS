/*
 * pongoOS - https://checkra.in
 *
 * Copyright (C) 2019-2023 checkra1n team
 *
 * This file is part of pongoOS.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#include <pongo.h>

extern volatile char gBootFlag;

/*

    Name: fdt_cmd
    Description: command handler for fdt

 */
extern void * fdt;
extern bool fdt_initialized;
extern char gLinuxCmdLine[LINUX_CMDLINE_SIZE];

void fdt_cmd() {
    if (!loader_xfer_recv_count) {
        iprintf("please upload a fdt before issuing this command\n");
        return;
    }
    if (fdt_initialized) free(fdt);
    fdt = malloc(LINUX_DTREE_SIZE);
    if (!fdt) panic("couldn't reserve heap for fdt");
    memcpy(fdt, loader_xfer_recv_data, loader_xfer_recv_count);
    fdt_initialized = 1;
    loader_xfer_recv_count = 0;
}

void linux_cmdline_cmd(const char* cmd, char* args) {
    if (!*args) {
        iprintf("linux_cmdline usage: linux_cmdline [cmdline]\n");
        return;
    }

    size_t len = strlen(args);
    if (len > LINUX_CMDLINE_SIZE) {
        iprintf("Provided command line length is greater than LINUX_CMDLINE_SIZE (%lu > %lu)\n", len, (size_t) LINUX_CMDLINE_SIZE);
        return;
    }

    memcpy(gLinuxCmdLine, args, len);
}

/*

    Name: pongo_boot_linux
    Description: command handler for bootl

*/

void pongo_boot_linux() {
    if (!linux_can_boot()) {
        printf("linux boot not prepared\n");
        return;
    }
    gBootFlag = BOOT_FLAG_LINUX;
    task_yield();
}

void dump_system_regs(const char * _arg1, char * _arg2) {
    unsigned long long buf;

    iprintf("-------- Feature registers --------\n");

    asm volatile ("mrs %0, ID_AA64ISAR0_EL1" : "=r"(buf) ::);
    iprintf("1. ID_AA64ISAR0_EL1: 0x%llx\n", buf);

    asm volatile ("mrs %0, ID_AA64PFR0_EL1" : "=r"(buf) ::);
    iprintf("2. ID_AA64PFR0_EL1: 0x%llx\n", buf);

    asm volatile ("mrs %0, ID_AA64PFR1_EL1" : "=r"(buf) ::);
    iprintf("3. ID_AA64PFR1_EL1: 0x%llx\n", buf);

    asm volatile ("mrs %0, MIDR_EL1" : "=r"(buf) ::);
    iprintf("4. MIDR_EL1: 0x%llx\n", buf);

    asm volatile ("mrs %0, ID_AA64ISAR1_EL1" : "=r"(buf) ::);
    iprintf("5. ID_AA64ISAR1_EL1: 0x%llx\n", buf);

    asm volatile ("mrs %0, ID_AA64MMFR0_EL1" : "=r"(buf) ::);
    iprintf("6. ID_AA64MMFR0_EL1: 0x%llx\n", buf);

    asm volatile ("mrs %0, ID_AA64MMFR1_EL1" : "=r"(buf) ::);
    iprintf("7. ID_AA64MMFR1_EL1: 0x%llx\n", buf);

    asm volatile ("mrs %0, ID_AA64MMFR2_EL1" : "=r"(buf) ::);
    iprintf("8. ID_AA64MMFR2_EL1: 0x%llx\n", buf);

    //apple clang doesnt like it probably because armv8.0 has no business having sve lol
    // asm volatile ("mrs %0, ID_AA64ZFR0_EL1" : "=r"(buf) ::);
    // iprintf("8. ID_AA64ZFR0_EL1: %llx\n", buf);


    iprintf("-------- Other regsiters --------\n");

    asm volatile ("mrs %0, CNTFRQ_EL0" : "=r"(buf) ::);
    iprintf("CNTFRQ_EL0: 0x%llx\n", buf);

    asm volatile ("mrs %0, CurrentEL" : "=r"(buf) ::);
    iprintf("CurrentEL [2:3]: 0x%llx\n", buf >> 2);

    asm volatile ("mrs %0, CLIDR_EL1" : "=r"(buf) ::);
    iprintf("CLIDR_EL1: 0x%llx\n", buf);
}

extern uint64_t gSynopsysBase;
#define DWC2_REG_VAL(x) *(volatile uint32_t *)(gSynopsysBase + x)

void dwc2_force_host_mode(bool want_host_mode) {
    unsigned long long GUSBCFG = DWC2_REG_VAL(0xc);

    int set = want_host_mode ? 29 : 30;
    int clear = want_host_mode ? 30 : 29;

    GUSBCFG &= ~(1ULL << clear);
    GUSBCFG |= (1ULL << set);

    *(volatile uint32_t *)(gSynopsysBase + 0xc) = GUSBCFG;

    sleep(1); // life is going so fast, take a single second to think about your choices and let dwc2 do the same.
}

void dump_usb_regs(const char * _arg1, char * _arg2) {
    unsigned long long GRXFSIZ = DWC2_REG_VAL(0x24);
    unsigned long long GNPTXFSIZ = DWC2_REG_VAL(0x28);
    unsigned long long GHWCFG1 = DWC2_REG_VAL(0x44);
    unsigned long long GHWCFG2 = DWC2_REG_VAL(0x48);
    unsigned long long GHWCFG3 = DWC2_REG_VAL(0x4c);
    unsigned long long GHWCFG4 = DWC2_REG_VAL(0x50);
    unsigned long long HPTXFSIZ = DWC2_REG_VAL(0x100);
    unsigned long long GSNPSID = DWC2_REG_VAL(0x400);

    // The order matches the one in /sys/kernel/debug/usb/devicename/hw_params
    iprintf("-------- DWC2 hw_params --------\n");

    iprintf("op_mode = 0x%llx\n", (GHWCFG2 & 0x7) >> 0);

    iprintf("arch = 0x%llx\n", (GHWCFG2 & (0x3 << 3)) >> 3);

    iprintf("dma_desc_enable = 0x%llx\n", GHWCFG4 & (1 << 30));

    iprintf("enable_dynamic_fifo = 0x%llx\n", GHWCFG2 & (1 << 19));

    iprintf("en_multiple_tx_fifo = 0x%llx\n", GHWCFG4 & (1 << 25));

    iprintf("rx_fifo_size (dont mistake with something_rx_fifo_size) = %llu\n", GRXFSIZ & 0xffff);

    /* force host mode to make the readout sane */
    dwc2_force_host_mode(true);
    iprintf("host_nperio_tx_fifo_size = %llu\n", (GNPTXFSIZ & (0xffff << 16)) >> 16);

    /* force dev mode to make the readout sane */
    dwc2_force_host_mode(false);
    iprintf("dev_nperio_tx_fifo_size = %llu\n", (GNPTXFSIZ & (0xffff << 16)) >> 16);

    iprintf("host_perio_tx_fifo_size = %llu\n", (HPTXFSIZ & (0xffff << 16)) >> 16);

    iprintf("nperio_tx_q_depth = %llu\n", (GHWCFG2 & (0x3 << 22)) >> 22 << 1);

    iprintf("host_perio_tx_q_depth = %llu\n", (GHWCFG2 & (0x3 << 24)) >> 24 << 1);

    iprintf("dev_token_q_depth = %llu\n", (GHWCFG2 & (0x1f << 26)) >> 26);

    unsigned long long width = (GHWCFG3 & 0xf);

    iprintf("max_transfer_size = %d\n", (1 << (width + 11)) - 1);

    width = ((GHWCFG3 & (0x7 << 4)) >> 4);

    iprintf("max_packet_size = %d\n", (1 << (width + 4)) - 1);

    iprintf("host_channels = %llu\n", 1 + ((GHWCFG2 & (0xf << 14)) >> 14));

    iprintf("hs_phy_type = %llu\n", ((GHWCFG2 & (0x3 << 6)) >> 6));

    iprintf("fs_phy_type = %llu\n", ((GHWCFG2 & (0x3 << 8)) >> 8));

    iprintf("i2c_enable = %llu\n", (GHWCFG3 & (1 << 8)));

    iprintf("num_dev_ep = %llu\n", ((GHWCFG2 & (0xf << 10))) >> 10);

    iprintf("num_dev_perio_in_ep = %llu\n", (GHWCFG4 & (0xf)));

    iprintf("total_fifo_size = %llu\n", ((GHWCFG3 & (0xffff << 16))) >> 16);

    iprintf("power_optimized = %llu\n", GHWCFG4 & (1 << 4));

    iprintf("utmi_phy_data_width = %llu\n", (GHWCFG4 & (0x3 << 14)) >> 14);

    iprintf("snpsid = 0x%llx\n", GSNPSID);

    iprintf("dev_ep_dirs = 0x%llx\n", GHWCFG1);
}

void linux_commands_register() {
    command_register("bootl", "boots linux", pongo_boot_linux);
    command_register("linux_cmdline", "update linux kernel command line", linux_cmdline_cmd);
    command_register("fdt", "load linux fdt from usb", fdt_cmd);
    command_register("dump_sysreg", "dump system register values", dump_system_regs);
    command_register("dump_usbreg", "dump USB register values", dump_system_regs);
}
