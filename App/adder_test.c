#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    unsigned long i, j;

    // Path to sysfs
    char reg_a_path[] = "/sys/bus/platform/drivers/my_calculator_drv/43c00000.gpio_test/oprd_a";
    char reg_b_path[] = "/sys/bus/platform/drivers/my_calculator_drv/43c00000.gpio_test/oprd_b";
    char reg_c_path[] = "/sys/bus/platform/drivers/my_calculator_drv/43c00000.gpio_test/output";

    // Sysfs accepts only strings
    char buf[sizeof(unsigned long) + 1];
    char output_buf[32];

    // File handler
    int fp_oprd_a;
    int fp_oprd_b;
    int fp_output;

    fp_oprd_a = open(reg_a_path, O_RDWR);
    if(fp_oprd_a == -1)
    {
        printf("Failed to open oprd_a\n");
        return 1;
    }

    fp_oprd_b = open(reg_b_path, O_RDWR);
    if(fp_oprd_b == -1)
    {
        close(fp_oprd_a);
        printf("Failed to open oprd_b\n");
        return 1;
    }

    fp_output = open(reg_c_path, O_RDONLY);
    if(fp_output == -1)
    {
        close(fp_oprd_a);
        close(fp_oprd_b);
        printf("Failed to open output\n");
        return 1;
    }

    // Initialize both operand registers to 0
    sprintf(buf, "%04d", 0);
    write(fp_oprd_a, buf, sizeof(unsigned long) + 1);
    write(fp_oprd_b, buf, sizeof(unsigned long) + 1);

    // Incrementing operand reg A
    for(i = 0; i < 16; i++)
    {
        sprintf(buf, "%04lu", i);
        printf("oprd_a: %lu\n", i);
        write(fp_oprd_a, buf, sizeof(unsigned long) + 1);

        // Incrementing operand reg B
        for(j = 0; j < 16; j++)
        {
            sprintf(buf, "%04lu", j);
            printf("oprd_b: %lu\n", j);
            write(fp_oprd_b, buf, sizeof(unsigned long) + 1);

            usleep(1000);

            // Read output register and check if it is correct
            pread(fp_output, output_buf, sizeof(unsigned long) + 1, 0);
            output_buf[sizeof(unsigned long)] = '\0';
            printf("Output: %s\n", output_buf);
            if((i + j) != (unsigned long)strtoul(output_buf, NULL, 16))
            {
                printf("reg A: %lu, reg B: %lu, output should be %lu, but %lu\n", i, j, i + j, (unsigned long)strtoul(output_buf, NULL, 16));
                close(fp_oprd_a);
                close(fp_oprd_b);
                close(fp_output);
                return 1;
            }
            usleep(300000);
        }
    }
    close(fp_oprd_a);
    close(fp_oprd_b);
    close(fp_output);
    return 0;
}
