#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

/* FreeRTOS */
#include <FreeRTOS.h>
#include <task.h>

/* aos */
#include <aos/kernel.h>
#include <cli.h>
#include <vfs.h>

/* RISCV */
#include <csi_core.h>

/* m1s_lfs_c906 */
#include <lfs_c906.h>

extern void cmd_c906_flash(char *buf, int len, int argc, char **argv);
const static struct cli_command cmds_user[] STATIC_CLI_CMD_ATTRIBUTE = {
    {"flash", "c906 flash command", cmd_c906_flash},
};

void main()
{
    lfs_register();

    int fd = -1;
    int ret = -1;
#define FILE_NAME "/lfs/boot_count"
    // read current count
    uint32_t boot_count = 0;
    if ((fd = aos_open(FILE_NAME, O_RDWR | O_CREAT)) < 0) {
        printf("%s not found!\r\n", FILE_NAME);
        return;
    }
    ret = aos_read(fd, &boot_count, sizeof(boot_count));
    if (ret < 0) {
        printf("read error %d\r\n", ret);
        return;
    }
    // update boot count
    boot_count += 1;
    ret = aos_lseek(fd, 0, SEEK_SET);
    if (ret != 0) {
        printf("lseek error %d\r\n", ret);
        return;
    }
    ret = aos_write(fd, &boot_count, sizeof(boot_count));
    if (ret < 0) {
        printf("write error %d\r\n", ret);
        return;
    }
    aos_close(fd);

    // print the boot count
    printf("boot_count: %d\n", boot_count);
}