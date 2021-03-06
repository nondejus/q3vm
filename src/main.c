/*
      ___   _______     ____  __
     / _ \ |___ /\ \   / /  \/  |
    | | | |  |_ \ \ \ / /| |\/| |
    | |_| |____) | \ V / | |  | |
     \__\_______/   \_/  |_|  |_|


   Quake III Arena Virtual Machine

   Standalone interpreter: load a .qvm file, run it, exit.
*/

#include <stdio.h>
#include <stdlib.h>
#include "vm.h"

/* The compiled bytecode calls native functions,
   defined in this file. */
intptr_t systemCalls(vm_t* vm, intptr_t* args);

/* Load an image from a file. Data is allocated with malloc.
   Call free() to unload image. */
uint8_t* loadImage(const char* filepath, int* size);

int main(int argc, char** argv)
{
    vm_t vm;
    int  retVal = -1;
    int  imageSize;

    if (argc < 2)
    {
        printf("No virtual machine supplied. Example: q3vm bytecode.qvm\n");
        return retVal;
    }

    char*    filepath = argv[1];
    uint8_t* image    = loadImage(filepath, &imageSize);
    if (!image)
    {
        return -1;
    }

    if (VM_Create(&vm, filepath, image, imageSize, systemCalls) == 0)
    {
        retVal = VM_Call(&vm, 0);
    }
    /* output profile information in DEBUG_VM build: */
    /* VM_VmProfile_f(&vm); */
    VM_Free(&vm);
    free(image); /* we can release the memory now */

    return retVal;
}

/* Callback from the VM that something went wrong */
void Com_Error(vmErrorCode_t level, const char* error)
{
    fprintf(stderr, "Err (%i): %s\n", level, error);
    exit(level);
}

/* Callback from the VM for memory allocation */
void* Com_malloc(size_t size, vm_t* vm, vmMallocType_t type)
{
    (void)vm;
    (void)type;
    return malloc(size);
}

/* Callback from the VM for memory release */
void Com_free(void* p, vm_t* vm, vmMallocType_t type)
{
    (void)vm;
    (void)type;
    free(p);
}

uint8_t* loadImage(const char* filepath, int* size)
{
    FILE*    f;            /* bytecode input file */
    uint8_t* image = NULL; /* bytecode buffer */
    int      sz;           /* bytecode file size */

    *size = 0;
    f     = fopen(filepath, "rb");
    if (!f)
    {
        fprintf(stderr, "Failed to open file %s.\n", filepath);
        return NULL;
    }
    /* calculate file size */
    fseek(f, 0L, SEEK_END);
    sz = ftell(f);
    if (sz < 1)
    {
        fclose(f);
        return NULL;
    }
    rewind(f);

    image = (uint8_t*)malloc(sz);
    if (!image)
    {
        fclose(f);
        return NULL;
    }

    if (fread(image, 1, sz, f) != (size_t)sz)
    {
        free(image);
        fclose(f);
        return NULL;
    }

    fclose(f);
    *size = sz;
    return image;
}

/* Callback from the VM: system function call */
intptr_t systemCalls(vm_t* vm, intptr_t* args)
{
    const int id = -1 - args[0];

    switch (id)
    {
    case -1: /* PRINTF */
        return printf("%s", (const char*)VMA(1, vm));

    case -2: /* ERROR */
        return fprintf(stderr, "%s", (const char*)VMA(1, vm));

    case -3: /* MEMSET */
        if (VM_MemoryRangeValid(args[1] /*addr*/, args[3] /*len*/, vm) == 0)
        {
            memset(VMA(1, vm), args[2], args[3]);
        }
        return args[1];

    case -4: /* MEMCPY */
        if (VM_MemoryRangeValid(args[1] /*addr*/, args[3] /*len*/, vm) == 0 &&
            VM_MemoryRangeValid(args[2] /*addr*/, args[3] /*len*/, vm) == 0)
        {
            memcpy(VMA(1, vm), VMA(2, vm), args[3]);
        }
        return args[1];

    default:
        fprintf(stderr, "Bad system call: %i\n", id);
    }
    return 0;
}
