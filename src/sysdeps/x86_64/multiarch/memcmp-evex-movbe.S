/* memcmp/wmemcmp optimized with 256-bit EVEX instructions.
   Copyright (C) 2021 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#if IS_IN (libc)

/* memcmp/wmemcmp is implemented as:
   1. For size from 2 to 7 bytes, load as big endian with movbe and bswap
      to avoid branches.
   2. Use overlapping compare to avoid branch.
   3. Use vector compare when size >= 4 bytes for memcmp or size >= 8
      bytes for wmemcmp.
   4. If size is 8 * VEC_SIZE or less, unroll the loop.
   5. Compare 4 * VEC_SIZE at a time with the aligned first memory
      area.
   6. Use 2 vector compares when size is 2 * VEC_SIZE or less.
   7. Use 4 vector compares when size is 4 * VEC_SIZE or less.
   8. Use 8 vector compares when size is 8 * VEC_SIZE or less.  */

# include <sysdep.h>

# ifndef MEMCMP
#  define MEMCMP	__memcmp_evex_movbe
# endif

# define VMOVU		vmovdqu64

# ifdef USE_AS_WMEMCMP
#  define VPCMPEQ	vpcmpeqd
# else
#  define VPCMPEQ	vpcmpeqb
# endif

# define XMM1		xmm17
# define XMM2		xmm18
# define YMM1		ymm17
# define YMM2		ymm18
# define YMM3		ymm19
# define YMM4		ymm20
# define YMM5		ymm21
# define YMM6		ymm22

# define VEC_SIZE 32
# ifdef USE_AS_WMEMCMP
#  define VEC_MASK 0xff
#  define XMM_MASK 0xf
# else
#  define VEC_MASK 0xffffffff
#  define XMM_MASK 0xffff
# endif

/* Warning!
           wmemcmp has to use SIGNED comparison for elements.
           memcmp has to use UNSIGNED comparison for elemnts.
*/

	.section .text.evex,"ax",@progbits
ENTRY (MEMCMP)
# ifdef USE_AS_WMEMCMP
	shl	$2, %RDX_LP
# elif defined __ILP32__
	/* Clear the upper 32 bits.  */
	movl	%edx, %edx
# endif
	cmp	$VEC_SIZE, %RDX_LP
	jb	L(less_vec)

	/* From VEC to 2 * VEC.  No branch when size == VEC_SIZE.  */
	VMOVU	(%rsi), %YMM2
	VPCMPEQ (%rdi), %YMM2, %k1
	kmovd	%k1, %eax
	subl    $VEC_MASK, %eax
	jnz	L(first_vec)

	cmpq	$(VEC_SIZE * 2), %rdx
	jbe	L(last_vec)

	/* More than 2 * VEC.  */
	cmpq	$(VEC_SIZE * 8), %rdx
	ja	L(more_8x_vec)
	cmpq	$(VEC_SIZE * 4), %rdx
	jb	L(last_4x_vec)

	/* From 4 * VEC to 8 * VEC, inclusively. */
	VMOVU	(%rsi), %YMM1
	VPCMPEQ (%rdi), %YMM1, %k1

	VMOVU	VEC_SIZE(%rsi), %YMM2
	VPCMPEQ VEC_SIZE(%rdi), %YMM2, %k2

	VMOVU	(VEC_SIZE * 2)(%rsi), %YMM3
	VPCMPEQ (VEC_SIZE * 2)(%rdi), %YMM3, %k3

	VMOVU	(VEC_SIZE * 3)(%rsi), %YMM4
	VPCMPEQ (VEC_SIZE * 3)(%rdi), %YMM4, %k4

	kandd	%k1, %k2, %k5
	kandd	%k3, %k4, %k6
	kandd	%k5, %k6, %k6

	kmovd	%k6, %eax
	cmpl	$VEC_MASK, %eax
	jne	L(4x_vec_end)

	leaq	-(4 * VEC_SIZE)(%rdi, %rdx), %rdi
	leaq	-(4 * VEC_SIZE)(%rsi, %rdx), %rsi
	VMOVU	(%rsi), %YMM1
	VPCMPEQ (%rdi), %YMM1, %k1

	VMOVU	VEC_SIZE(%rsi), %YMM2
	VPCMPEQ VEC_SIZE(%rdi), %YMM2, %k2
	kandd	%k1, %k2, %k5

	VMOVU	(VEC_SIZE * 2)(%rsi), %YMM3
	VPCMPEQ (VEC_SIZE * 2)(%rdi), %YMM3, %k3
	kandd	%k3, %k5, %k5

	VMOVU	(VEC_SIZE * 3)(%rsi), %YMM4
	VPCMPEQ (VEC_SIZE * 3)(%rdi), %YMM4, %k4
	kandd	%k4, %k5, %k5

	kmovd	%k5, %eax
	cmpl	$VEC_MASK, %eax
	jne	L(4x_vec_end)
	xorl	%eax, %eax
	ret

	.p2align 4
L(last_2x_vec):
	/* From VEC to 2 * VEC.  No branch when size == VEC_SIZE.  */
	VMOVU	(%rsi), %YMM2
	VPCMPEQ (%rdi), %YMM2, %k2
	kmovd	%k2, %eax
	subl    $VEC_MASK, %eax
	jnz	L(first_vec)

L(last_vec):
	/* Use overlapping loads to avoid branches.  */
	leaq	-VEC_SIZE(%rdi, %rdx), %rdi
	leaq	-VEC_SIZE(%rsi, %rdx), %rsi
	VMOVU	(%rsi), %YMM2
	VPCMPEQ (%rdi), %YMM2, %k2
	kmovd	%k2, %eax
	subl    $VEC_MASK, %eax
	jnz	L(first_vec)
	ret

	.p2align 4
L(first_vec):
	/* A byte or int32 is different within 16 or 32 bytes.  */
	tzcntl	%eax, %ecx
# ifdef USE_AS_WMEMCMP
	xorl	%eax, %eax
	movl	(%rdi, %rcx, 4), %edx
	cmpl	(%rsi, %rcx, 4), %edx
L(wmemcmp_return):
	setl	%al
	negl	%eax
	orl	$1, %eax
# else
	movzbl	(%rdi, %rcx), %eax
	movzbl	(%rsi, %rcx), %edx
	sub	%edx, %eax
# endif
	ret

# ifdef USE_AS_WMEMCMP
	.p2align 4
L(4):
	xorl	%eax, %eax
	movl	(%rdi), %edx
	cmpl	(%rsi), %edx
	jne	L(wmemcmp_return)
	ret
# else
	.p2align 4
L(between_4_7):
	/* Load as big endian with overlapping movbe to avoid branches.  */
	movbe	(%rdi), %eax
	movbe	(%rsi), %ecx
	shlq	$32, %rax
	shlq	$32, %rcx
	movbe	-4(%rdi, %rdx), %edi
	movbe	-4(%rsi, %rdx), %esi
	orq	%rdi, %rax
	orq	%rsi, %rcx
	subq	%rcx, %rax
	je	L(exit)
	sbbl	%eax, %eax
	orl	$1, %eax
	ret

	.p2align 4
L(exit):
	ret

	.p2align 4
L(between_2_3):
	/* Load as big endian to avoid branches.  */
	movzwl	(%rdi), %eax
	movzwl	(%rsi), %ecx
	shll	$8, %eax
	shll	$8, %ecx
	bswap	%eax
	bswap	%ecx
	movb	-1(%rdi, %rdx), %al
	movb	-1(%rsi, %rdx), %cl
	/* Subtraction is okay because the upper 8 bits are zero.  */
	subl	%ecx, %eax
	ret

	.p2align 4
L(1):
	movzbl	(%rdi), %eax
	movzbl	(%rsi), %ecx
	subl	%ecx, %eax
	ret
# endif

	.p2align 4
L(zero):
	xorl	%eax, %eax
	ret

	.p2align 4
L(less_vec):
# ifdef USE_AS_WMEMCMP
	/* It can only be 0, 4, 8, 12, 16, 20, 24, 28 bytes.  */
	cmpb	$4, %dl
	je	L(4)
	jb	L(zero)
# else
	cmpb	$1, %dl
	je	L(1)
	jb	L(zero)
	cmpb	$4, %dl
	jb	L(between_2_3)
	cmpb	$8, %dl
	jb	L(between_4_7)
# endif
	cmpb	$16, %dl
	jae	L(between_16_31)
	/* It is between 8 and 15 bytes.  */
	vmovq	(%rdi), %XMM1
	vmovq	(%rsi), %XMM2
	VPCMPEQ %XMM1, %XMM2, %k2
	kmovw	%k2, %eax
	subl    $XMM_MASK, %eax
	jnz	L(first_vec)
	/* Use overlapping loads to avoid branches.  */
	leaq	-8(%rdi, %rdx), %rdi
	leaq	-8(%rsi, %rdx), %rsi
	vmovq	(%rdi), %XMM1
	vmovq	(%rsi), %XMM2
	VPCMPEQ %XMM1, %XMM2, %k2
	kmovw	%k2, %eax
	subl    $XMM_MASK, %eax
	jnz	L(first_vec)
	ret

	.p2align 4
L(between_16_31):
	/* From 16 to 31 bytes.  No branch when size == 16.  */
	VMOVU	(%rsi), %XMM2
	VPCMPEQ (%rdi), %XMM2, %k2
	kmovw	%k2, %eax
	subl    $XMM_MASK, %eax
	jnz	L(first_vec)

	/* Use overlapping loads to avoid branches.  */
	leaq	-16(%rdi, %rdx), %rdi
	leaq	-16(%rsi, %rdx), %rsi
	VMOVU	(%rsi), %XMM2
	VPCMPEQ (%rdi), %XMM2, %k2
	kmovw	%k2, %eax
	subl    $XMM_MASK, %eax
	jnz	L(first_vec)
	ret

	.p2align 4
L(more_8x_vec):
	/* More than 8 * VEC.  Check the first VEC.  */
	VMOVU	(%rsi), %YMM2
	VPCMPEQ (%rdi), %YMM2, %k2
	kmovd	%k2, %eax
	subl    $VEC_MASK, %eax
	jnz	L(first_vec)

	/* Align the first memory area for aligned loads in the loop.
	   Compute how much the first memory area is misaligned.  */
	movq	%rdi, %rcx
	andl	$(VEC_SIZE - 1), %ecx
	/* Get the negative of offset for alignment.  */
	subq	$VEC_SIZE, %rcx
	/* Adjust the second memory area.  */
	subq	%rcx, %rsi
	/* Adjust the first memory area which should be aligned now.  */
	subq	%rcx, %rdi
	/* Adjust length.  */
	addq	%rcx, %rdx

L(loop_4x_vec):
	/* Compare 4 * VEC at a time forward.  */
	VMOVU	(%rsi), %YMM1
	VPCMPEQ (%rdi), %YMM1, %k1

	VMOVU	VEC_SIZE(%rsi), %YMM2
	VPCMPEQ VEC_SIZE(%rdi), %YMM2, %k2
	kandd	%k2, %k1, %k5

	VMOVU	(VEC_SIZE * 2)(%rsi), %YMM3
	VPCMPEQ (VEC_SIZE * 2)(%rdi), %YMM3, %k3
	kandd	%k3, %k5, %k5

	VMOVU	(VEC_SIZE * 3)(%rsi), %YMM4
	VPCMPEQ (VEC_SIZE * 3)(%rdi), %YMM4, %k4
	kandd	%k4, %k5, %k5

	kmovd	%k5, %eax
	cmpl	$VEC_MASK, %eax
	jne	L(4x_vec_end)

	addq	$(VEC_SIZE * 4), %rdi
	addq	$(VEC_SIZE * 4), %rsi

	subq	$(VEC_SIZE * 4), %rdx
	cmpq	$(VEC_SIZE * 4), %rdx
	jae	L(loop_4x_vec)

	/* Less than 4 * VEC.  */
	cmpq	$VEC_SIZE, %rdx
	jbe	L(last_vec)
	cmpq	$(VEC_SIZE * 2), %rdx
	jbe	L(last_2x_vec)

L(last_4x_vec):
	/* From 2 * VEC to 4 * VEC. */
	VMOVU	(%rsi), %YMM2
	VPCMPEQ (%rdi), %YMM2, %k2
	kmovd	%k2, %eax
	subl    $VEC_MASK, %eax
	jnz	L(first_vec)

	addq	$VEC_SIZE, %rdi
	addq	$VEC_SIZE, %rsi
	VMOVU	(%rsi), %YMM2
	VPCMPEQ (%rdi), %YMM2, %k2
	kmovd	%k2, %eax
	subl    $VEC_MASK, %eax
	jnz	L(first_vec)

	/* Use overlapping loads to avoid branches.  */
	leaq	-(3 * VEC_SIZE)(%rdi, %rdx), %rdi
	leaq	-(3 * VEC_SIZE)(%rsi, %rdx), %rsi
	VMOVU	(%rsi), %YMM2
	VPCMPEQ (%rdi), %YMM2, %k2
	kmovd	%k2, %eax
	subl    $VEC_MASK, %eax
	jnz	L(first_vec)

	addq	$VEC_SIZE, %rdi
	addq	$VEC_SIZE, %rsi
	VMOVU	(%rsi), %YMM2
	VPCMPEQ (%rdi), %YMM2, %k2
	kmovd	%k2, %eax
	subl    $VEC_MASK, %eax
	jnz	L(first_vec)
	ret

	.p2align 4
L(4x_vec_end):
	kmovd	%k1, %eax
	subl	$VEC_MASK, %eax
	jnz	L(first_vec)
	kmovd	%k2, %eax
	subl	$VEC_MASK, %eax
	jnz	L(first_vec_x1)
	kmovd	%k3, %eax
	subl	$VEC_MASK, %eax
	jnz	L(first_vec_x2)
	kmovd	%k4, %eax
	subl	$VEC_MASK, %eax
	tzcntl	%eax, %ecx
# ifdef USE_AS_WMEMCMP
	xorl	%eax, %eax
	movl	(VEC_SIZE * 3)(%rdi, %rcx, 4), %edx
	cmpl	(VEC_SIZE * 3)(%rsi, %rcx, 4), %edx
	jmp	L(wmemcmp_return)
# else
	movzbl	(VEC_SIZE * 3)(%rdi, %rcx), %eax
	movzbl	(VEC_SIZE * 3)(%rsi, %rcx), %edx
	sub	%edx, %eax
# endif
	ret

	.p2align 4
L(first_vec_x1):
	tzcntl	%eax, %ecx
# ifdef USE_AS_WMEMCMP
	xorl	%eax, %eax
	movl	VEC_SIZE(%rdi, %rcx, 4), %edx
	cmpl	VEC_SIZE(%rsi, %rcx, 4), %edx
	jmp	L(wmemcmp_return)
# else
	movzbl	VEC_SIZE(%rdi, %rcx), %eax
	movzbl	VEC_SIZE(%rsi, %rcx), %edx
	sub	%edx, %eax
# endif
	ret

	.p2align 4
L(first_vec_x2):
	tzcntl	%eax, %ecx
# ifdef USE_AS_WMEMCMP
	xorl	%eax, %eax
	movl	(VEC_SIZE * 2)(%rdi, %rcx, 4), %edx
	cmpl	(VEC_SIZE * 2)(%rsi, %rcx, 4), %edx
	jmp	L(wmemcmp_return)
# else
	movzbl	(VEC_SIZE * 2)(%rdi, %rcx), %eax
	movzbl	(VEC_SIZE * 2)(%rsi, %rcx), %edx
	sub	%edx, %eax
# endif
	ret
END (MEMCMP)
#endif
