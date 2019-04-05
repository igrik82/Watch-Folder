#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>


#define SETTING_PATH "/etc/watchFolders.conf"
#define BUFFER 512

//Структура с настройками программы
struct settings{
    char *directory;
    unsigned amountTime;
    char unitOfTime;
};

//Структура стека для возможности удаления в первую очередь файлов а только затем папок
struct stack{
    char *directory;
    struct stack *next;
};

struct stack *listDirRecursively(char *basePath, struct stack *stackHolder);
FILE *readSettings(char *settingPath);
struct settings *parseStringSetting(char *strSetting);
int toDelete(char *directory, unsigned amountTime, char unitOfTime);
struct stack *pushToStack(char *directory, struct stack *stackHolder);
char *popFromStack(struct stack **stackHolder);
char *getPathToTrash(char *path);
char *getCleanDirPath(char *string);

int main() {
    //Получаем handle файла с настройками
    FILE *fileHandler = NULL;
    fileHandler = readSettings(SETTING_PATH);
    if(!fileHandler){
        return -1;
    }

    char buffer[BUFFER] = {'\0'};

    //читаем в буфер построчно исключая дирректории "." и ".."
    while ((fgets(buffer, BUFFER, fileHandler) != NULL)) {
        if ((strncmp(buffer, "#", 1)) != 0 && (strncmp(buffer, "\n", 1)) != 0) {

            //Создаем и получаем hаndle структуры и после парсим настройки посточно
            struct settings *settingHandler = NULL;
            settingHandler = parseStringSetting(buffer);

            if(!settingHandler){
                return -1;
            }

            //Инициализируем стек holder
            struct stack *stackHolder = NULL;
            stackHolder = listDirRecursively(settingHandler->directory, stackHolder);

            while (stackHolder != NULL){
                char *tempNameHolder = popFromStack(&stackHolder);

                int flag = toDelete(tempNameHolder, settingHandler->amountTime, settingHandler->unitOfTime);

                //Если функция toDelete помечает как к удалению
                if(flag != 0){
                    //если Trash то удаляем, если другая папка то перемещаем в корзину
                    if(strstr(tempNameHolder, ".Trash") != NULL){
                        remove(tempNameHolder);
                    } else{
                        char *tempTrashHolder = getPathToTrash(tempNameHolder);
                        rename(tempNameHolder, tempTrashHolder);
                        free(tempTrashHolder);
                        tempTrashHolder = NULL;
                    }
                }

                free(tempNameHolder);
                tempNameHolder = NULL;
            }

            //Отдельно освобождаем выделенную память под названия каталога, которую выделили на этапе парсинга
            free(settingHandler->directory);
            settingHandler->directory = NULL;
            free(settingHandler);
            settingHandler = NULL;
        }
    }
    fclose(fileHandler);
}

char *getPathToTrash(char *path){

    // Узнаем длину строки пути файла и получаем указатель последнего вхождения символа '/' посредством адресной
    // арифметики. Можно конечно этого добиться через функции строк самого языка, но этот метод мне показался более
    // красивым. Фрагмент пути равен длине + '\0' и также после этого имеем указатель (ptr) на искомое.
    int totalLength, fragmentLength;
    char *ptr = path;
    while(*ptr){
        ptr++;
    }
    totalLength = ptr - path;
    while (*ptr != '/'){
        ptr--;
    }
    fragmentLength = totalLength - (ptr - path);

    int pathLength = strlen(getenv("HOME"));

    //   /.Trash == 7
    char *pathToTrash = (char*)malloc((pathLength + 7 + fragmentLength) * sizeof(char));

    strcpy(pathToTrash, getenv("HOME"));
    strcat(pathToTrash, "/.Trash");
    strcat(pathToTrash, ptr);

    return pathToTrash;
}



/* Реализация извлечения из стека строки. Для того, чтобы не возвращать структуру и иметь возможность производить ее
 * непосредственное изменение при вызове функции, выполнил двойное взятие адреса(указатель на указатель). */
char *popFromStack(struct stack **stackHolder){

    if(!stackHolder){
        return NULL;
    }

    struct stack *tempStackHolder = NULL;

    // Для возможности освобождения памяти берем адрес.
    char *tempHolder = NULL;
    tempStackHolder = *stackHolder;

    tempHolder = (*stackHolder)->directory;
    *stackHolder = (*stackHolder)->next;

    // Освобождаем ноду.
    free(tempStackHolder);
    tempStackHolder = NULL;

    return tempHolder;
}

/* Реализация помещения в стек пути к файлу(папке). Возвращает структуру стека. */
struct stack *pushToStack(char *directory, struct stack *stackHolder){

    if(!stackHolder){
        stackHolder = (struct stack*)malloc(sizeof(struct stack));
        stackHolder->directory = directory;
        stackHolder->next = NULL;
        return stackHolder;
    } else {
        struct stack *newNode = (struct stack*)malloc(sizeof(struct stack));
        newNode->directory = directory;
        newNode->next = stackHolder;
        return newNode;
    }

}

int toDelete(char *directory, unsigned amountTime, char unitOfTime){
    //Узнаем текущее время
    time_t seconds;
    seconds = time(NULL);

    //Узнаем время последнего доступа
    struct stat *timeHandler = (struct stat*)malloc(sizeof(struct stat));
    stat(directory, timeHandler);

    long timeDifference = 1;

    if(amountTime > 1000000000 || amountTime < 0){
        return 0;
    }
    switch (unitOfTime){
        case 's':
            timeDifference = amountTime - (seconds - timeHandler->st_mtimespec.tv_sec);
            break;
        case 'm':
            timeDifference = (amountTime * 60) - (seconds - timeHandler->st_mtimespec.tv_sec);
            break;
        case 'h':
            timeDifference = (amountTime * 60 * 60) - (seconds - timeHandler->st_mtimespec.tv_sec);
            break;
        case 'd':
            timeDifference = (amountTime * 60 * 60 * 24) - (seconds - timeHandler->st_mtimespec.tv_sec);
            break;
        default:
            break;
    }

    free(timeHandler);
    if(timeDifference < 0){
        return 1;
    } else {
        return 0;
    }
}

FILE *readSettings(char *settingPath){
    FILE *file_ptr;
    file_ptr = fopen(settingPath, "r");
    if(file_ptr){
        return file_ptr;
    }
    return NULL;
}

/* Функция очищает строку пути от начальных и крайних пробелов и '/'. Возвращает строку.*/
char *getCleanDirPath(char *givenString){

    //Для возможности изменения копируем строку в массив
    char modifString[BUFFER] = {'\0'};
    strcpy(modifString, givenString);
    // Избавляемся от лишних пробелов в начале
    char *str_ptr = givenString;
    while (*str_ptr == ' '){
        str_ptr++;
    }
    int i = 0;
    while(*str_ptr){
        modifString[i++] = *str_ptr++;
    }
    str_ptr = &modifString[--i];

    // Избавляемся от лишних пробелов в конце
    if(*str_ptr == ' '){
        while (*str_ptr == ' '){
            str_ptr--;
        }
    }
    //Проверяем на слеш если стоим на нем, то заменяем на нулевой символ,
    //если нет то сдвигаемся на один символ вправо и присваиваем там.
    if(*str_ptr == '/'){
        *str_ptr = '\0';
    } else {
        *(++str_ptr) = '\0';
    }

    // Вычисляем длину строки
    str_ptr = modifString;
    while (*str_ptr++){
        ;
    }
    int totalCount = str_ptr - modifString;

    //Выделяем память под конечную строку
    char *cleanDirPath = (char*)calloc(totalCount, sizeof(char));
    strcpy(cleanDirPath, modifString);
    return cleanDirPath;
}

/* Функция парсинга строки из файла настроек. Возвращает структуру настроек */
struct settings *parseStringSetting(char *strSetting){
    char unitOfTime;
    unsigned amountTime, counter, spaceCounter = 0;
    char *str_ptr = strSetting;

    // Идем до конца строки.
    while (*str_ptr != '\n'){
        str_ptr++;
    }
    // Встаем на пробел и доходим до символа unitOfTime
    --str_ptr;
    if(*str_ptr == ' '){
        while(*str_ptr == ' '){
            str_ptr--;
        }
    }
    unitOfTime = *str_ptr;

    // Встаем на пробел и переходим на amountTime и преобразуем в число
    str_ptr--;
    while(*str_ptr == ' '){
        str_ptr--;
    }
    while (*str_ptr != ' '){
        str_ptr--;
    }
    *str_ptr++ = '\0';
    char *end;
    amountTime = strtol(str_ptr, &end, 10);

    //Получаем читый путь с экранированием пробелов и проверкой на лишние пробелы в начале и конце, а также слеш символ
    char *directory = getCleanDirPath(strSetting);

    struct settings *oneStrSetting = (struct settings*)malloc(sizeof(struct settings));

    if(!oneStrSetting){
        return NULL;
    }

    oneStrSetting->directory = directory;
    oneStrSetting->amountTime = amountTime;
    oneStrSetting->unitOfTime = unitOfTime;

    return oneStrSetting;
}

struct stack *listDirRecursively(char *basePath, struct stack *stackHolder){
    struct dirent *entry;
    DIR *folder_ptr = opendir(basePath);

    if(!folder_ptr){
        return NULL;
    }

    //Перебираем папки которые находятся в переданном пути, кроме "." ".."
    while ((entry = readdir(folder_ptr)) != NULL){
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0){
            // Выделяем память под базовый путь, имя файла, слеш, и нулевой символ
            // инициализируем нулевыми символами с помощью calloс
            int strBasePart = strlen(basePath);
            int strLength = strlen(getCleanDirPath(entry->d_name));
            char *fullDirPath = (char*)calloc(strBasePart + strLength + 2, sizeof(char));

            //Собираем полный путь
            strcpy(fullDirPath, basePath);
            strcat(fullDirPath, "/");
            strcat(fullDirPath, getCleanDirPath(entry->d_name));

            //Заталкиваем пути в стек
            stackHolder = pushToStack(fullDirPath, stackHolder);

            /*Если у файла тип 4 то это папка (полное определение в файле sys/dirent.h)
             * и это папка .Trash то рекурсивно вызываем свою функцию*/
            if((strstr(basePath, ".Trash") != NULL) && entry->d_type == 4)
                stackHolder = listDirRecursively(fullDirPath, stackHolder);
        }
    }
    closedir(folder_ptr);
    return stackHolder;
}