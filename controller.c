#include "controller.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>  // snprintf için gerekli

char *handle_input(ShmBuf *shmp, const char *input) {
    if (input == NULL) {
        return strdup("");
    }

    // Trim whitespace
    while (isspace(*input)) input++;

    if (strncmp(input, "@msg", 4) == 0 && isspace(input[4])) {
        const char *rest = input + 4;
        while (isspace(*rest)) rest++;

        // Hedef terminal ID'yi al
        char target[10];
        int i = 0;
        while (*rest && !isspace(*rest) && i < 9) {
            target[i++] = *rest++;
        }
        target[i] = '\0';

        while (isspace(*rest)) rest++;
        if (*rest == '\0') return strdup("Error: Empty message");

        char formatted[1024];
        snprintf(formatted, sizeof(formatted), "[to:%s] %s", target, rest);
        model_send_message(shmp, formatted);
        return strdup("Message sent.");
    } else {
        return execute_command(input);
    }
}
