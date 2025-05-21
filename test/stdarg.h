#ifndef STDARG_H
#define STDARG_H

/* 
 * Variadic function argument handling for NCC
 * This is a simplified implementation for 16-bit x86
 */

typedef unsigned char* va_list;

/* 
 * va_start - Initialize a va_list to point to the first variable argument
 * Parameters:
 *   ap: The va_list to initialize
 *   last: The last named parameter before the variable arguments
 */
#define va_start(ap, last) ((ap) = (va_list)&(last) + sizeof(last))

/*
 * va_arg - Get the next argument from a va_list
 * Parameters:
 *   ap: The va_list to get the argument from
 *   type: The type of the argument to get
 * Returns: The next argument from the va_list, cast to the specified type
 */
#define va_arg(ap, type) (*(type*)((ap += sizeof(type)) - sizeof(type)))

/*
 * va_copy - Copy one va_list to another
 * Parameters:
 *   dest: The destination va_list
 *   src: The source va_list to copy
 */
#define va_copy(dest, src) ((dest) = (src))

/*
 * va_end - Clean up a va_list
 * Parameters:
 *   ap: The va_list to clean up
 */
#define va_end(ap) ((ap) = (va_list)0)

#endif /* STDARG_H */
