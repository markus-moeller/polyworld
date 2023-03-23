
#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int link(char *linkName, char *target) {
    int result = -1;
    BOOLEAN success = CreateSymbolicLinkA(linkName, target, 0x0);
    if (success) {
        result = 0;
    }
    return result;
}
