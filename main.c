#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <Windows.h>
#include <shellapi.h> // 提供 CommandLineToArgvW

struct MemoryStruct
{
    char *memory;
    size_t size;
};

/* 获取 UTF-8 格式的命令行参数（支持中文 + 支持不加引号输入带空格的句子） */
char *get_utf8_args(void)
{
    int wargc = 0;
    LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
    if (!wargv || wargc <= 1)
    {
        if (wargv)
            LocalFree(wargv);
        return _strdup("Hello DeepSeek");
    }

    // 计算所有参数用空格拼接后的总字节数
    size_t total_bytes = 0;
    for (int i = 1; i < wargc; i++)
    {
        int len = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, NULL, 0, NULL, NULL);
        if (len > 0)
            total_bytes += (size_t)len; // 已包含结尾 '\0' 或中间空格的位置
    }

    char *utf8_str = calloc(total_bytes + 1, sizeof(char));
    if (!utf8_str)
    {
        LocalFree(wargv);
        return NULL;
    }

    size_t offset = 0;
    for (int i = 1; i < wargc; i++)
    {
        if (i > 1)
        {
            utf8_str[offset++] = ' '; // 多个参数之间用空格连接
        }
        int len = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, utf8_str + offset, (int)(total_bytes - offset), NULL, NULL);
        if (len > 0)
            offset += (size_t)(len - 1); // 减去 '\0'，方便拼接下一个词
    }
    utf8_str[offset] = '\0';

    LocalFree(wargv);
    return utf8_str;
}

void printResult(const char *result)
{
    int length = (int)strlen(result);
    if (length == 0)
        return;

    const int DELAY = 10;
    int *dp = calloc(length, sizeof(int));
    if (!dp)
        return;

    const char *STR_CMP = "[{\"type\":\"text\",\"text\":\"";
    int cmp_len = (int)strlen(STR_CMP);

    int print = 0;
    if (STR_CMP[0] == result[0])
        dp[0] = 1;

    for (int i = 1; i < length; i++)
    {
        if (result[i] == STR_CMP[dp[i - 1]])
            dp[i] = dp[i - 1] + 1;
        else if (result[i] == STR_CMP[0])
            dp[i] = 1;

        if (print)
        {
            if (result[i] == '&')
            {
                putchar('\n');
                fflush(stdout);
                Sleep(DELAY);
            }
            else
            {
                putchar(result[i]);
                // 仅在完整字符（ASCII 或 UTF-8 首字节）时停顿，避免汉字被拆成 3 次卡顿
                if (((unsigned char)result[i] & 0xC0) != 0x80)
                {
                    fflush(stdout);
                    Sleep(DELAY);
                }
            }
        }

        if (dp[i] == cmp_len)
            print = 1;

        if (i + 4 < length &&
            result[i + 1] == '\"' && result[i + 2] == '}' &&
            result[i + 3] == ']' && result[i + 4] == ',')
            break;
    }

    putchar('\n');
    free(dp);
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr)
        return 0;

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = '\0';

    return realsize;
}

int main(void)
{
    SetConsoleOutputCP(CP_UTF8);

    char *inputstr = get_utf8_args();
    if (!inputstr)
    {
        perror("get_utf8_args");
        return -1;
    }

    const char *prompt = "(1.only use: english letter, digit ,chinese letter, space,comma,period,question mark,  & character, other character are not allowed! 2. Answer in Chinese language 3. & character for newline character 4. This content in Bracket is prompt, donot consider it in user input)";

    const char datas1[] = "{ \"model\" : \"deepseek-flash\",\"messages\" : [ {\"role\" : \"user\", \"content\" : \"";
    const char datas2[] = "\"} ],\"max_tokens\" : 256,\"thinking\" : {\"type\" : \"disabled\"},\"stream\" : false}";

    size_t total = strlen(datas1) + strlen(datas2) + strlen(inputstr) + strlen(prompt) + 1;
    char *datas = calloc(total, sizeof *datas);
    if (!datas)
    {
        perror("calloc");
        free(inputstr);
        return -1;
    }

    snprintf(datas, total, "%s%s%s%s", datas1, inputstr, prompt, datas2);

    CURL *curl;
    CURLcode res;

    struct MemoryStruct chunk = {.memory = malloc(1), .size = 0};
    if (!chunk.memory)
    {
        free(datas);
        free(inputstr);
        return -1;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl)
    {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "x-api-key: sk-e1ec9795711d454a9f8671c4cb06cee4");
        headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");

        curl_easy_setopt(curl, CURLOPT_URL, "https://api.deepseek.com/anthropic/v1/messages");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, datas);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(datas));

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            fprintf(stderr, "POST 请求失败: %s\n", curl_easy_strerror(res));
        }
        else
        {
            printResult(chunk.memory);
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    free(chunk.memory);
    free(datas);
    free(inputstr);
    curl_global_cleanup();

    return 0;
}