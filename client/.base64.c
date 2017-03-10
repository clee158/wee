#include ".base64.h"

static const unsigned char base64_table[65] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const int B64index[256] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62, 63, 62, 62, 63, 52, 53, 54, 55,
56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,
7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,
0,  0,  0, 63,  0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };

char *get_ip() {
	FILE *file = fopen(".ip_addr", "r");
	char *line = malloc(sizeof(char) * 22);
	fgets(line, 22, file);
	line[21] = '\0';
	fclose(file);

	return base64_decode(line, 23);
}

char *get_port() {
	FILE *file = fopen(".ip_port", "r");
	char *line = malloc(sizeof(char) * 9);
	fgets(line, 9, file);
	line[8] = '\0';
	fclose(file);

	return base64_decode(line, 23);
}

char *base64_decode(char* data, size_t len) {
   	char *p = (char *)data;
    int pad = len > 0 && (len % 4 || p[len - 1] == '=');
    size_t L = ((len + 3) / 4 - pad) * 4;
		
    char *str = calloc(1, L / 4 * 3 + pad);
	
		size_t i, j;
    for (i = 0, j = 0; i < L; i += 4) {
        int n = B64index[(int)p[i]] << 18 | B64index[(int)p[i + 1]] << 12 | B64index[(int)p[i + 2]] << 6 | B64index[(int)p[i + 3]];
        str[j++] = n >> 16;
        str[j++] = n >> 8 & 0xFF;
        str[j++] = n & 0xFF;
    }

    if (pad) {
        int n = B64index[(int)p[L]] << 18 | B64index[(int)p[L + 1]] << 12;
        str[strlen(str) - 1] = n >> 16;

        if (len > L + 2 && p[L + 2] != '=') {
            n |= B64index[(int)p[L + 2]] << 6;
						str[strlen(str)] = n >> 8 & 0xFF;
        }
    }

    return str;
}
/**
 * base64_decode - Base64 decode
 * @src: Data to be decoded
 * @len: Length of the data to be decoded
 * @out_len: Pointer to output length variable
 * Returns: Allocated buffer of out_len bytes of decoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer.
char *base64_decode(char *src, size_t len) {
	char dtable[256], *out, *pos, block[4], tmp;
	size_t i, count, olen;
	int pad = 0;

	memset(dtable, 0x80, 256);
	for (i = 0; i < sizeof(base64_table) - 1; i++)
		dtable[(int)base64_table[i]] = i;
	dtable['='] = 0;

	count = 0;
	for (i = 0; i < len; i++) {
		if (dtable[(int)src[i]] != 0x80)
			count++;
	}

	if (count == 0 || count % 4)
		return NULL;

	olen = count / 4 * 3;
	pos = out = malloc(olen);
	if (out == NULL)
		return NULL;

	count = 0;
	for (i = 0; i < len; i++) {
		tmp = dtable[(int)src[i]];
		if (tmp == 0x80)
			continue;

		if (src[i] == '=')
			pad++;
		block[count] = tmp;
		count++;
		if (count == 4) {
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			*pos++ = (block[2] << 6) | block[3];
			count = 0;
			if (pad) {
				if (pad == 1)
					pos--;
				else if (pad == 2)
					pos -= 2;
				else {
					free(out);
					return NULL;
				}
				break;
			}
		}
	}

	return out;
}
 */
