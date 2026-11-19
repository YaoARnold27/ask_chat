#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <Windows.h>

struct MemoryStruct
{
    char *memory;
    size_t size;
};

void *printResult(char *result)
{

    printf("%s\n", result);

    int length = strlen(result);
    const int DELAY = 10;
    int *dp = calloc(length, sizeof(int));

    int a = length, b = 0;

    char *STR_CMP = "[{\"type\":\"text\",\"text\":\"";

    int print = 0;
    if (STR_CMP[0] == result[0])
        dp[0] = 1;
    // gothrough
    for (int i = 1; i < length; i++)
    {
        if (result[i] == STR_CMP[dp[i - 1]])
            dp[i] = dp[i - 1] + 1;

        if (print)
        {
            if (result[i] == '&')
            {
                putchar('\n');
                
                Sleep(DELAY);
            }
            else
            {
                putchar(result[i]);
                Sleep(DELAY);
            }
        }

        if (dp[i] == strlen(STR_CMP))
            print = 1;

        if (result[i + 1] == '\"' && result[i + 2] == '}' && result[i + 3] == ']' && result[i + 4] == ',')
            break;
    }

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

int main(int argc, char *argv[])
{
    char *inputstr = "Hello DeepSeek";
    char *prompt = "(1.Only use Chinese Englsh space ? and & character) 2. Answer in Chinese language 3. & character for newline character)";
    // parse args
    if (argc > 1)
        inputstr = argv[1];

    SetConsoleOutputCP(CP_UTF8);

    char datas1[] = "{ \"model\" : \"deepseek-flash\",\"messages\" : [ {\"role\" : \"user\", \"content\" : \"";
    char datas2[] = "\"} ],\"max_tokens\" : 32,\"thinking\" : {\"type\" : \"disabled\"},\"stream\" : false}";

    size_t total = strlen(datas1) + strlen(datas2) + strlen(inputstr) + strlen(prompt) + 3;
    char *datas = calloc(total, sizeof *datas);
    if (!datas)
    {
        perror("calloc");
        return -1;
    }

    // concat the string
    int counter = 0;
    for (int i = 0; i < strlen(datas1); i++)
    {
        datas[counter] = datas1[i];
        counter++;
    }
    for (int i = 0; i < strlen(inputstr); i++)
    {
        datas[counter] = inputstr[i];
        counter++;
    }
    for (int i = 0; i < strlen(prompt); i++)
    {
        datas[counter] = prompt[i];
        counter++;
    }
    for (int i = 0; i < strlen(datas2); i++)
    {
        datas[counter] = datas2[i];
        counter++;
    }

    datas[counter] = '\0';

    CURL *curl;
    CURLcode res;

    struct MemoryStruct chunk = {.memory = malloc(1), .size = 0};

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl)
    {
        /* 设置自定义请求头 */
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "x-api-key: sk-e1ec9795711d454a9f8671c4cb06cee4");

        headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");

        curl_easy_setopt(curl, CURLOPT_URL, "https://api.deepseek.com/anthropic/v1/messages");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        /* 配置 POST 参数 */
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, datas);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(datas));

        /* 配置响应接收 */
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

        /* 释放链表和 curl 句柄 */
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    free(chunk.memory);
    free(datas);
    curl_global_cleanup();

    return 0;
}
