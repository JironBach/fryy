#include "shell.h"
#define BEGIN_CMD() int task_func = _task_func
#define END_CMD() if (task_func == 0) task_remove(task_get()); else return
#define ADD_CMD(cmd)\
    if (strncmp(#cmd, buffer, strlen(#cmd)) == 0)\
        return cmd_##cmd##;

static int _task_func = 0;
static dentry_t cd;
static char buffer[BUFSZ];
static void cmd_echo();
static void cmd_author();
static void cmd_halt();
static void cmd_ps();
static void cmd_cd();
static int ehandler_cd(dentry_t * entry);
static void cmd_dir();
static int ehandler_dir(dentry_t * entry);
static void cmd_cat();
static int ehandler_cat(dentry_t * entry);
static int shandler_cat(char * sector, int length);
static void (*find_procedure(char * cmd))();
static void cmd_print();
extern char sector[SECTOR_SIZE];

void shell()
{
    char c;
    cd.fstClus = 0;
    while (1)
    {
        int p = 0;
        void (*f)();
        puts("$> ");
        while (1)
        {
            c = getc();
            if (c == 13)
            {
                buffer[p++] = '\0';
                ENTER();
                break;
            }
            else if (c == 8)
            {
                if (p > 0)
                {
                    p--;
                    putc(c);
                    putc(' ');
                    putc(c);
                }
            }
            else
            {
                putc(c);
                buffer[p++] = c;
            }
        }
        if ((f = find_procedure(buffer)) == 0)
        {
            puts("command not found!");
            ENTER();
            continue;
        }
        else
        {
            if (buffer[p-2] == '&')
            {
                p -= 2;
                buffer[p] = '\0';
                _task_func = 0;
                task_create(f, KERNELBASE);
            }
            else
            {
                _task_func = 1;
                f();
            }
        }
    }
}
static void (*find_procedure(char * cmd))()
{
    ADD_CMD(echo);
    ADD_CMD(author);
    ADD_CMD(dir);
    ADD_CMD(cd);
    ADD_CMD(cat);
    ADD_CMD(print);
    ADD_CMD(ps);
    ADD_CMD(halt);
    return 0;
}

static void cmd_echo()
{
    int i;
    BEGIN_CMD();
    for(i = 5; buffer[i] != '\0'; i++)
        putc(buffer[i]);
    ENTER();
    END_CMD();
}

static void cmd_author()
{
    BEGIN_CMD();

    char *author;
    author = "Junya Shimoda\0";
    printf("%s", author);

    ENTER();
    END_CMD();
}

static void cmd_halt()
{
    /* Kill tasks other than shell 
     * excepts there only be the shell
     * task
     */
    tcb_t * tcb = 0;
    asm "mov ax, #1"; // task_get
    asm "int 0x21";
    asm "mov word -6[bp], ax"; // return value
    while (tcb->tid)
        tcb = tcb->next;
    //task_remove(tcb);
    asm "push word -6[bp]"; // pass tcb to syscall
    asm "mov ax, #0"; // task_remove
    asm "int 0x21";
}

static void cmd_dir()
{
    BEGIN_CMD();
    fs_dir_read(&cd, ehandler_dir);
    END_CMD();
}

static void cmd_ps()
{
    BEGIN_CMD();
    tcb_t * current, * target;
    target = task_get();
    current = target;
    do
    {
        print(current->tid);ENTER();
        current = current->next;
    }
    while(current != target);
    END_CMD();
}

static int ehandler_dir(dentry_t * entry)
{
    if (IS_FREE(entry))
        return 0;
    if (ATTR_DIRECTORY(entry) || ATTR_ARCHIVE(entry))
    {
        puts(entry->name);
        ENTER();
    }
    return 0;
}

static void cmd_cd()
{
    BEGIN_CMD();
    if (fs_dir_read(&cd, ehandler_cd))
        END_CMD();
    puts("No such directory!");
    ENTER();
    END_CMD();
}
static int ehandler_cd(dentry_t * entry)
{
    if (IS_FREE(entry))
        return 0;
    if (ATTR_DIRECTORY(entry) || ATTR_ARCHIVE(entry))
        if (strncmp(entry->name, buffer+3, strlen(buffer+3)) == 0)
        {
            cd = *entry;
            return 1;
        }
    return 0;
}

static void cmd_cat()
{
    BEGIN_CMD();
    if (fs_dir_read(&cd, ehandler_cat))
        END_CMD();
    puts("No such file!");
    ENTER();
    END_CMD();
}
static int ehandler_cat(dentry_t * entry)
{
    if (IS_FREE(entry))
        return 0;
    if (ATTR_DIRECTORY(entry) || ATTR_ARCHIVE(entry))
        if (strncmp(entry->name, buffer+4, strlen(buffer+4)) == 0)
        {
            fs_file_read(entry, shandler_cat);
            ENTER();
            return 1;
        }
    return 0;
}

static int shandler_cat(char * sector, int length)
{
    int i;
    for (i = 0; i < length; i++)
        putc(sector[i]);
    return 0;
}

static void cmd_print()
{
    BEGIN_CMD();
    char c = *(buffer + 6);
    while (1)
        putc(c);
    END_CMD();
}
