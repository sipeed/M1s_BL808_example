#include <stdio.h>

/* FreeRTOS */
#include <FreeRTOS.h>
#include <task.h>

/* aos */
#include <cli.h>

/* fs */
#include <bl_romfs.h>

/* RISCV */
#include <csi_core.h>

extern void cmd_c906_flash(char *buf, int len, int argc, char **argv);
const static struct cli_command cmds_user[] STATIC_CLI_CMD_ATTRIBUTE = {
    {"flash", "c906 flash command", cmd_c906_flash},
};

void main() 
{
    romfs_register();
}