#include "Grow.h"

typedef struct {
    A_u_long index;
    A_char   str[256];
} TableString;

static TableString g_strs[1] = {
    { 0, "" }
};
