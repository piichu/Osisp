#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>

extern char **environ;

#define VAR_NUM 7

char **getFileVariables(const char *fileName)
{
    FILE *f = NULL;
    if ((f = fopen(fileName, "r")) != NULL)
    {
        char **result = (char **)calloc(VAR_NUM, sizeof(char *));
        for (int i = 0; i < VAR_NUM; i++)
        {
            result[i] = (char *)calloc(80, sizeof(char));
            if (fgets(result[i], 80, f) != NULL)
            {
                result[i][strcspn(result[i], "\n")] = '\0'; // удаляем символ перевода строки
            }
        }
        fclose(f);
        return result;
    }
    printf("Error: can't open file\n");
    return NULL;
}

int includeString(const char *str1, const char *str2)
{
    for (size_t i = 0; i < strlen(str2) - 1; i++)
        if (str1[i] != str2[i])
            return 0;
    return 1;
}

char *getEnvpVariable(char *envp[], const char *variable)
{
    int i = 0;
    while (envp[i])
    {
        if (includeString(envp[i], variable))
            break;
        i++;
    }
    return envp[i];
}

int main(int argc, char *argv[], char *envp[])
{

    if (argc < 2)
    {
        printf("Error: not enough arguments passed to child.\n");
        exit(1);
    }

    printf("Process name: %s\nProcess pid: %d\nProcess ppdid: %d\n", argv[0], (int)getpid(), (int)getppid());

    // Чтение названий переменных среды из переданного файла
    char **variables = getFileVariables(argv[1]);
    if (variables != NULL)
    {
        // Получение значений переменных среды
        switch (argv[2][0])
        {
        case '+':
            for (int i = 0; i < VAR_NUM; i++)
                // Поиск среди переменных среды из getenv
                printf("%s=%s\n", variables[i], getenv(variables[i]));
            break;
        case '*':
            for (int i = 0; i < VAR_NUM; i++)
            {
                // Поиск по названию в envp
                char *response = getEnvpVariable(envp, variables[i]);
                if (response != NULL)
                    printf("%s\n", response);
            }
            break;
        case '&':
            for (int i = 0; i < VAR_NUM; i++)
            {
                // Поиск по названию в environ
                char *response = getEnvpVariable(environ, variables[i]);
                if (response != NULL)
                    printf("%s\n", response);
            }
            break;
        }
    }

    return 0;
}