#include "../lib/shell.h"
#include "../lib/fs.h"
#include "../lib/disk.h"

int
main(int argc, char **argv)
{
    Shell shell;
    shell.run();
    return 0;
}
