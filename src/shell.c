#include <common.h>
#include <tasking.h>
#include <acpi.h>
#include <memory.h>
#include <hal.h>
#include <rtc.h>
#include <shell.h>
#include <main.h>
#include <fs.h>

char* currentDir;
void shell_main()
{
    if(!currentDir)
    {
        free(currentDir);
    }
    char* dir = "/mnt/";
    currentDir = malloc(128);
    memcpy(currentDir, dir, strlen(dir));
    send_sig(p_id(), SIG_PRI, THREAD_PRIORITY_REALTIME);
    sleep(1000);
    while(1)
    {
        if(textscreen_getcursorx() != 0)
        {
            printf("\n");
        }
        printf("CMD>>");
        char* input;
        scanf("%s", &input, 20);
        process_cmd(input);
        free(input);
    }
}

void process_cmd(char* input)
{
    if(textscreen_getcursorx() != 0)
    {
        printf("\n");
    }
    if(strcmp(input, "halt"))
    {
        acpi_shutdown();
    }
    else if(strcmp(input, "reboot"))
    {
        acpi_reboot();
    }
    else if(strcmp(input, "clear"))
    {
        textscreen_clear();
        textscreen_setcursor(0, 0);
    }
    else if(strbegins(input, "echo "))
    {
        textscreen_clear();
    }
    else if(strcmp(input, "ps"))
    {
        printf("TID       State   Name\n");
        for(int i = 0; i < listlength(thread_list); i++)
        {
            printf("%d     %d       %s\n", ((thread_t*)thread_list->pointer[i])->tid, ((thread_t*)thread_list->pointer[i])->state, ((thread_t*)thread_list->pointer[i])->name);
        }
    }
    else if(strcmp(input, "date"))
    {
        char* c = get_current_datetime_str();
        printf("%s\n", c);
        free(c);
    }
    else if(strcmp(input, "ls"))
    {
        fdirs(currentDir);
    }
    else if(strbegins(input, "cd "))
    {
        if(strcmp(input + 3, ".."))
        {
            if(!strcmp(currentDir, "/"))
            {
                int32_t point = strlastindex(currentDir, '/');
                currentDir[point] = 0;
                int32_t last = strlastindex(currentDir, '/') + 1;
                memset(currentDir + last, 0, strlen(currentDir) - last);
            }
        }
        else
        {
            uint32_t before = strlen(currentDir);
            uint32_t initial = before;
            memcpy(currentDir + before, input + 3, strlen(input + 3));
            before = strlen(currentDir);
            char* ptr = currentDir + before;
            ptr[0] = '/';
            if(fexists(currentDir) != 1)
            {
                printf("Could not find directory %s\n", currentDir);
                memset(currentDir + initial, 0, 128 - initial);
            }
        }
    }
    else if(strbegins(input, "cat "))
    {
        uint32_t before = strlen(currentDir);
        uint32_t initial = before;
        memcpy(currentDir + before, input + 4, strlen(input + 4));
        before = strlen(currentDir);
        char* ptr = currentDir + before;
        ptr[0] = '/';
        if(fexists(currentDir) == 2)
        {
            file_t* file = fopen(currentDir);
            if(file != null)
            {
                uint8_t* buf = malloc(513);
                while(fread(buf, 512, file))
                {
                    printf("%s", buf);
                    memset(buf, 0, 512);
                }
                free(buf);
                fclose(file);
            }
        }
        memset(currentDir + initial, 0, 128 - initial);
    }
    else if(strbegins(input, "load "))
    {
        uint32_t before = strlen(currentDir);
        uint32_t initial = before;
        memcpy(currentDir + before, input + 5, strlen(input + 5));
        before = strlen(currentDir);
        char* ptr = currentDir + before;
        ptr[0] = '/';
        if(fexists(currentDir) == 2)
        {
            file_t* file = fopen(currentDir);
            if(file != null)
            {
                uint8_t* buf = malloc(file->size);
                fread(buf, file->size, file);
                create_thread(file->name, (uint32_t)buf);
                fclose(file);
            }
        }
        memset(currentDir + initial, 0, 128 - initial);
    }
    else if(strcmp(input, "tree"))
    {
        flist(vfs_root_node);
    }
    else if(strcmp(input, "pwd"))
    {
        printf(currentDir);
    }
    else if(strcmp(input, "lshw"))
    {
        for(int32_t i = 0; i < listlength(devices); i++)
        {
            device_t* dev;
            peekitem(devices, i, (uint32_t*)&dev);
            printf("%s\n", dev->name);
        }
    }
    else if(strcmp(input, "lsmod"))
    {
        printf("%d Modules Loaded\n", multiboot->mods_count);
        if (multiboot->mods_count > 0)
        {
            for (uint32_t i = 0; i < multiboot->mods_count; ++i )
            {
                module_t* mod = (module_t*)((uint32_t*)multiboot->mods_addr + (8 * i));
                printf("%s\n", mod->name);
            }
        }
    }
    else if(strbegins(input, "loop "))
    {
        input += 5;
        char* num = malloc(30);
        int32_t depth = 0;
        while(input[depth] != ' ' && input[depth] != 0)
        {
            num[depth] = input[depth];
            depth ++;
        }
        input += depth + 1;
        depth = atoi(num, "%d");
        free(num);
        for(int32_t i = 0; i < depth; i++)
        {
            process_cmd(input);
        }
    }
    else if(strbegins(input, "kill "))
    {
        uint32_t pid = atoi(input + 5, "%d");
        if(pid == 0)
        {
            for(int i = 0; i < listlength(thread_list); i++)
            {
                if(strcmp(((thread_t*)thread_list->pointer[i])->name, input + 5))
                {
                    pid = ((thread_t*)thread_list->pointer[i])->tid;
                }
            }
        }
        send_sig(pid, SIG_TERM, 0);
    }
    else if(strbegins(input, "memprint "))
    {
        uint32_t start = atoi(input + 9, "%x");
        uint32_t i = 9;
        for(; i < strlen(input); i++)
        {
            if(input[i] == ' ')
            {
                i++;
                break;
            }
        }
        if(i != strlen(input))
        {
            uint32_t count = atoi(input + i);
            uint8_t* ptr = (uint8_t*)start;
            for(uint32_t i = 0; i < count; i++)
            {
                printf("0x%x ", ptr[i]);
            }
            printf("\n");
        }
    }
    else if(strbegins(input, "memset "))
    {
        uint32_t start = atoi(input + 7, "%x");
        uint32_t i = 7;
        for(; i < strlen(input); i++)
        {
            if(input[i] == ' ')
            {
                i++;
                break;
            }
        }
        if(i != strlen(input))
        {
            uint32_t count = atoi(input + i);
            for(; i < strlen(input); i++)
            {
                if(input[i] == ' ')
                {
                    i++;
                    break;
                }
            }
            uint8_t value = (uint8_t)atoi(input + i);
            uint8_t* ptr = (uint8_t*)start;
            for(uint32_t i = 0; i < count; i++)
            {
                *ptr = value;
                ptr++;
            }
            printf("\n");
        }
    }
    else if(strcmp(input, "test"))
    {
        printf("Test What\n");
        free(input);
        scanf("%s", &input, 20);
        if(textscreen_getcursorx() != 0)
        {
            printf("\n");
        }
        if(strcmp(input, "mem"))
        {
            printf("Running memory test for the next 10 seconds\n");
            for(int i = 0; i < 10; i++)
            {
                void* alloc = malloc(((i % 2) * 100) + 100);
                printf("Test %d allocated %d at %d\n", i, ((i % 2) * 100) + 100, alloc);
                free(alloc);
                sleep(1000);
            }
        }
        else if(strcmp(input, "virus"))
        {
            list_t* ids = makelist(listlength(thread_list));
            sleep(1);
            for(int i = 0; i < listlength(thread_list); i++)
            {
                thread_t* thread;
                peekitem(thread_list, i, (uint32_t*)&thread);
                send_sig(thread->tid, SIG_TERM, 0);
                listadd(ids, thread->tid);
            }
            sleep(1);
            for(int i = 0; i < listlength(ids); i++)
            {
                uint32_t id;
                peekitem(ids, i, &id);
                send_sig(id, SIG_TERM, 0);
            }
            free(ids);
        }
        else
        {
            printf("No test found\n");
        }
    }
    else
    {
        if(strlen(input) > 0)
        {
            printf("Bad Command or File Name (%s)\n", input);
        }
    }
    if(textscreen_getcursorx() != 0)
    {
        printf("\n");
    }
}