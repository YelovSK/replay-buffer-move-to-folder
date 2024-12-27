#include <string-utils.h>

void replace_char(wchar_t *str, wchar_t target, wchar_t replacement)
{
	while (*str) {
		if (*str == target) {
			*str = replacement;
		}
		str++;
	}
}
