/* crypto/ec/ecp_smpl.c */
/* Includes code written by Lenka Fibikova <fibikova@exp-math.uni-essen.de>
 * for the OpenSSL project. */
/* ====================================================================
 * Copyright (c) 1998-2001 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */

#include <openssl/err.h>

#include "ec_lcl.h"


const EC_METHOD *EC_GFp_simple_method(void)
	{
	static const EC_METHOD ret = {
		ec_GFp_simple_group_init,
		ec_GFp_simple_group_set_curve_GFp,
		ec_GFp_simple_group_finish,
		ec_GFp_simple_group_clear_finish,
		ec_GFp_simple_group_copy,
		ec_GFp_simple_group_set_generator,
		/* TODO: 'set' and 'get' functions for EC_GROUPs */
		ec_GFp_simple_point_init,
		ec_GFp_simple_point_finish,
		ec_GFp_simple_point_clear_finish,
		ec_GFp_simple_point_copy,
		/* TODO: 'set' and 'get' functions for EC_POINTs */
		ec_GFp_simple_point2oct,
		ec_GFp_simple_oct2point,
		ec_GFp_simple_add,
		ec_GFp_simple_dbl,
		ec_GFp_simple_is_at_infinity,
		ec_GFp_simple_is_on_curve,
		ec_GFp_simple_make_affine,
		ec_GFp_simple_field_mul,
		ec_GFp_simple_field_sqr,
		0 /* field_encode */,
		0 /* field_decode */ };

	return &ret;
	}


int ec_GFp_simple_group_init(EC_GROUP *group)
	{
	BN_init(&group->field);
	BN_init(&group->a);
	BN_init(&group->b);
	group->a_is_minus3 = 0;
	group->generator = NULL;
	BN_init(&group->order);
	BN_init(&group->cofactor);
	return 1;
	}


int ec_GFp_simple_group_set_curve_GFp(EC_GROUP *group,
	const BIGNUM *p, const BIGNUM *a, const BIGNUM *b, BN_CTX *ctx)
	{
	int ret = 0;
	BN_CTX *new_ctx = NULL;
	BIGNUM *tmp_a;
	
	if (ctx == NULL)
		{
		ctx = new_ctx = BN_CTX_new();
		if (ctx == NULL)
			return 0;
		}
	BN_CTX_start(ctx);

	tmp_a = BN_CTX_get(ctx);
	if (tmp_a == NULL) goto err;

	/* group->field */
	if (!BN_copy(&group->field, p)) goto err;
	group->field.neg = 0;

	/* group->a */
	if (!BN_nnmod(tmp_a, a, p, ctx)) goto err;
	if (group->meth->field_encode)
		{ if (!group->meth->field_encode(group, &group->a, tmp_a, ctx)) goto err; }	
	else
		if (!BN_copy(&group->a, tmp_a)) goto err;
	
	/* group->b */
	if (!BN_nnmod(&group->b, b, p, ctx)) goto err;
	if (group->meth->field_encode)
		if (!group->meth->field_encode(group, &group->b, &group->b, ctx)) goto err;
	
	/* group->a_is_minus3 */
	if (!BN_add_word(tmp_a, 3)) goto err;
	group->a_is_minus3 = (0 == BN_cmp(tmp_a, &group->field));

	ret = 1;

 err:
	BN_CTX_end(ctx);
	if (new_ctx != NULL)
		BN_CTX_free(new_ctx);
	return ret;
	}


void ec_GFp_simple_group_finish(EC_GROUP *group)
	{
	BN_free(&group->field);
	BN_free(&group->a);
	BN_free(&group->b);
	if (group->generator != NULL)
		EC_POINT_free(group->generator);
	BN_free(&group->order);
	BN_free(&group->cofactor);
	}


void ec_GFp_simple_group_clear_finish(EC_GROUP *group)
	{
	BN_clear_free(&group->field);
	BN_clear_free(&group->a);
	BN_clear_free(&group->b);
	if (group->generator != NULL)
		{
		EC_POINT_clear_free(group->generator);
		group->generator = NULL;
		}
	BN_clear_free(&group->order);
	BN_clear_free(&group->cofactor);
	}


int ec_GFp_simple_group_copy(EC_GROUP *dest, const EC_GROUP *src)
	{
	if (!BN_copy(&dest->field, &src->field)) return 0;
	if (!BN_copy(&dest->a, &src->a)) return 0;
	if (!BN_copy(&dest->b, &src->b)) return 0;

	dest->a_is_minus3 = src->a_is_minus3;

	if (src->generator != NULL)
		{
		if (dest->generator == NULL)
			{
			dest->generator = EC_POINT_new(dest);
			if (dest->generator == NULL) return 0;
			}
		if (!EC_POINT_copy(dest->generator, src->generator)) return 0;
		}
	else
		{
		/* src->generator == NULL */
		if (dest->generator != NULL)
			{
			EC_POINT_clear_free(dest->generator);
			dest->generator = NULL;
			}
		}

	if (!BN_copy(&dest->order, &src->order)) return 0;
	if (!BN_copy(&dest->cofactor, &src->cofactor)) return 0;

	return 1;
	}


int ec_GFp_simple_group_set_generator(EC_GROUP *group, const EC_POINT *generator,
	const BIGNUM *order, const BIGNUM *cofactor)
	{
	if (generator)
		{
		ECerr(EC_F_EC_GFP_SIMPLE_GROUP_SET_GENERATOR, ERR_R_PASSED_NULL_PARAMETER);
		return 0   ;
		}

	if (group->generator == NULL)
		{
		group->generator = EC_POINT_new(group);
		if (group->generator == NULL) return 0;
		}
	if (!EC_POINT_copy(group->generator, generator)) return 0;

	if (order != NULL)
		{ if (!BN_copy(&group->order, order)) return 0; }	
	else
		{ if (!BN_zero(&group->order)) return 0; }	

	if (cofactor != NULL)
		{ if (!BN_copy(&group->cofactor, cofactor)) return 0; }	
	else
		{ if (!BN_zero(&group->cofactor)) return 0; }	

	return 1;
	}


/* TODO: 'set' and 'get' functions for EC_GROUPs */


int ec_GFp_simple_point_init(EC_POINT *point)
	{
	BN_init(&point->X);
	BN_init(&point->Y);
	BN_init(&point->Z);
	point->Z_is_one = 0;

	return 1;
	}


void ec_GFp_simple_point_finish(EC_POINT *point)
	{
	BN_free(&point->X);
	BN_free(&point->Y);
	BN_free(&point->Z);
	}


void ec_GFp_simple_point_clear_finish(EC_POINT *point)
	{
	BN_clear_free(&point->X);
	BN_clear_free(&point->Y);
	BN_clear_free(&point->Z);
	point->Z_is_one = 0;
	}


int ec_GFp_simple_point_copy(EC_POINT *dest, const EC_POINT *src)
	{
	if (!BN_copy(&dest->X, &src->X)) return 0;
	if (!BN_copy(&dest->Y, &src->Y)) return 0;
	if (!BN_copy(&dest->Z, &src->Z)) return 0;
	dest->Z_is_one = src->Z_is_one;

	return 1;
	}


/* TODO: 'set' and 'get' functions for EC_POINTs */


size_t ec_GFp_simple_point2oct(const EC_GROUP *group, const EC_POINT *point, point_conversion_form_t form,
	unsigned char *buf, size_t len, BN_CTX *ctx);
/* TODO */


int ec_GFp_simple_oct2point(const EC_GROUP *group, EC_POINT *point,
	const unsigned char *buf, size_t len, BN_CTX *);
/* TODO */


int ec_GFp_simple_add(const EC_GROUP *group, EC_POINT *r, const EC_POINT *a, const EC_POINT *b, BN_CTX *ctx)
	{
	int (*field_mul)(const EC_GROUP *, BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *);
	int (*field_sqr)(const EC_GROUP *, BIGNUM *, const BIGNUM *, BN_CTX *);
	const BIGNUM *p;
	BN_CTX *new_ctx = NULL;
	BIGNUM *n0, *n1, *n2, *n3, *n4, *n5, *n6;
	int ret = 0;
	
	if (a == b)
		return EC_POINT_dbl(group, r, a, ctx);
	if (EC_POINT_is_at_infinity(group, a))
		return EC_POINT_copy(r, b);
	if (EC_POINT_is_at_infinity(group, b))
		return EC_POINT_copy(r, a);
	
	field_mul = group->meth->field_mul;
	field_sqr = group->meth->field_sqr;
	p = &group->field;

	if (ctx == NULL)
		{
		ctx = new_ctx = BN_CTX_new();
		if (ctx == NULL)
			return 0;
		}
	BN_CTX_start(ctx);

	n0 = BN_CTX_get(ctx);
	n1 = BN_CTX_get(ctx);
	n2 = BN_CTX_get(ctx);
	n3 = BN_CTX_get(ctx);
	n4 = BN_CTX_get(ctx);
	n5 = BN_CTX_get(ctx);
	n6 = BN_CTX_get(ctx);
	if (n6 == NULL) goto end;

	/* n1, n2 */
	if (b->Z_is_one)
		{
		if (!BN_copy(n1, &a->X)) goto end;
		if (!BN_copy(n2, &a->Y)) goto end;
		/* n1 = X_a */
		/* n2 = Y_a */
		}
	else
		{
		if (!field_sqr(group, n0, &b->Z, ctx)) goto end;
		if (!field_mul(group, n1, &a->X, n0, ctx)) goto end;
		/* n1 = X_a * Z_b^2 */

		if (!field_mul(group, n0, n0, &b->Z, ctx)) goto end;
		if (!field_mul(group, n2, &a->Y, n0, ctx)) goto end;
		/* n2 = Y_a * Z_b^3 */
		}

	/* n3, n4 */
	if (a->Z_is_one)
		{
		if (!BN_copy(n3, &b->X)) goto end;
		if (!BN_copy(n4, &b->Y)) goto end;
		/* n3 = X_b */
		/* n4 = Y_b */
		}
	else
		{
		if (!field_sqr(group, n0, &a->Z, ctx)) goto end;
		if (!field_mul(group, n3, &b->X, n0, ctx)) goto end;
		/* n3 = X_b * Z_a^2 */

		if (!field_mul(group, n0, n0, &a->Z, ctx)) goto end;
		if (!field_mul(group, n4, &b->Y, n0, ctx)) goto end;
		/* n4 = Y_b * Z_a^3 */
		}

	/* n5, n6 */
	if (!BN_mod_sub_quick(n5, n1, n3, p)) goto end;
	if (!BN_mod_sub_quick(n6, n2, n4, p)) goto end;
	/* n5 = n1 - n3 */
	/* n6 = n2 - n4 */

	if (BN_is_zero(n5))
		{
		if (BN_is_zero(n6))
			{
			/* a is the same point as b */
			BN_CTX_end(ctx);
			ctx = NULL;
			ret = EC_POINT_dbl(group, r, a, ctx);
			goto end;
			}
		else
			{
			/* a is the inverse of b */
			if (!BN_zero(&r->Z)) goto end;
			r->Z_is_one = 0;
			ret = 1;
			goto end;
			}
		}

	/* 'n7', 'n8' */
	if (!BN_mod_add_quick(n1, n1, n3, p)) goto end;
	if (!BN_mod_add_quick(n2, n2, n4, p)) goto end;
	/* 'n7' = n1 + n3 */
	/* 'n8' = n2 + n4 */

	/* Z_r */
	if (a->Z_is_one && b->Z_is_one)
		{
		if (!BN_copy(&r->Z, n5)) goto end;
		}
	else
		{
		if (a->Z_is_one)
			{ if (!BN_copy(n0, &b->Z)) goto end; }
		else if (b->Z_is_one)
			{ if (!BN_copy(n0, &a->Z)) goto end; }
		else
			{ if (!field_mul(group, n0, &a->Z, &b->Z, ctx)) goto end; }
		if (!field_mul(group, &r->Z, n0, n5, ctx)) goto end;
		}
	r->Z_is_one = 0;
	/* Z_r = Z_a * Z_b * n5 */

	/* X_r */
	if (!field_sqr(group, n0, n6, ctx)) goto end;
	if (!field_sqr(group, n4, n5, ctx)) goto end;
	if (!field_mul(group, n3, n1, n4, ctx)) goto end;
	if (!BN_mod_sub_quick(&r->X, n0, n3, p)) goto end;
	/* X_r = n6^2 - n5^2 * 'n7' */
	
	/* 'n9' */
	if (!BN_mod_lshift1_quick(n0, &r->X, p)) goto end;
	if (!BN_mod_sub_quick(n0, n3, n0, p)) goto end;
	/* n9 = n5^2 * 'n7' - 2 * X_r */

	/* Y_r */
	if (!field_mul(group, n0, n0, n6, ctx)) goto end;
	if (!field_mul(group, n5, n4, n5, ctx)) goto end; /* now n5 is n5^3 */
	if (!field_mul(group, n1, n2, n5, ctx)) goto end;
	if (!BN_mod_sub_quick(n0, n0, n1, p)) goto end;
	if (BN_is_odd(n0))
		if (!BN_add(n0, n0, p)) goto end;
	/* now  0 <= n0 < 2*p,  and n0 is even */
	if (!BN_rshift1(&r->Y, n0)) goto end;
	/* Y_r = (n6 * 'n9' - 'n8' * 'n5^3') / 2 */

	ret = 1;

 end:
	if (ctx) /* otherwise we already called BN_CTX_end */
		BN_CTX_end(ctx);
	if (new_ctx != NULL)
		BN_CTX_free(new_ctx);
	return ret;
	}


int ec_GFp_simple_dbl(const EC_GROUP *group, EC_POINT *r, const EC_POINT *a, BN_CTX *ctx)
	{
	int (*field_mul)(const EC_GROUP *, BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *);
	int (*field_sqr)(const EC_GROUP *, BIGNUM *, const BIGNUM *, BN_CTX *);
	const BIGNUM *p;
	BN_CTX *new_ctx = NULL;
	BIGNUM *n0, *n1, *n2, *n3;
	int ret = 0;
	
	if (EC_POINT_is_at_infinity(group, a))
		{
		if (!BN_zero(&r->Z)) return 0;
		r->Z_is_one = 0;
		return 1;
		}

	field_mul = group->meth->field_mul;
	field_sqr = group->meth->field_sqr;
	p = &group->field;

	if (ctx == NULL)
		{
		ctx = new_ctx = BN_CTX_new();
		if (ctx == NULL)
			return 0;
		}
	BN_CTX_start(ctx);

	n0 = BN_CTX_get(ctx);
	n1 = BN_CTX_get(ctx);
	n2 = BN_CTX_get(ctx);
	n3 = BN_CTX_get(ctx);
	if (n3 == NULL) goto err;

	/* TODO: optimization for group->a_is_minus3 */

	/* n1 */
	if (a->Z_is_one)
		{
		if (!field_sqr(group, n0, &a->X, ctx)) goto err;
		if (!BN_mod_lshift1_quick(n1, n0, p)) goto err;
		if (!BN_mod_add_quick(n0, n0, n1, p)) goto err;
		if (!BN_mod_add_quick(n1, n0, &group->a, p)) goto err;
		/* n1 = 3 * X_a^2 + a_curve */
		}
	else if (group->a_is_minus3)
		{
		if (!field_sqr(group, n1, &a->Z, ctx)) goto err;
		if (!BN_mod_add_quick(n0, &a->X, n1, p)) goto err;
		if (!BN_mod_sub_quick(n2, &a->X, n1, p)) goto err;
		if (!field_mul(group, n1, n0, n2, ctx)) goto err;
		if (!BN_mod_lshift1_quick(n0, n1, p)) goto err;
		if (!BN_mod_add_quick(n1, n0, n1, p)) goto err;
		/* n1 = 3 * (X_a + Z_a^2) * (X_a - Z_a^2)
		 *    = 3 * X_a^2 - 3 * Z_a^4 */
		}
	else
		{
		if (!field_sqr(group, n0, &a->X, ctx)) goto err;
		if (!BN_mod_lshift1_quick(n1, n0, p)) goto err;
		if (!BN_mod_add_quick(n0, n0, n1, p)) goto err;
		if (!field_sqr(group, n1, &a->Z, ctx)) goto err;
		if (!field_sqr(group, n1, n1, ctx)) goto err;
		if (!field_mul(group, n1, n1, &group->a, ctx)) goto err;
		if (!BN_mod_add_quick(n1, n1, n0, p)) goto err;
		/* n1 = 3 * X_a^2 + a_curve * Z_a^4 */
		}

	/* Z_r */
	if (a->Z_is_one)
		{
		if (!BN_copy(n0, &a->Y)) goto err;
		}
	else
		{
		if (!field_mul(group, n0, &a->Y, &a->Z, ctx)) goto err;
		}
	if (!BN_mod_lshift1_quick(&r->Z, n0, p)) goto err;
	r->Z_is_one = 0;
	/* Z_r = 2 * Y_a * Z_a */

	/* n2 */
	if (!field_sqr(group, n3, &a->Y, ctx)) goto err;
	if (!field_mul(group, n2, &a->X, n3, ctx)) goto err;
	if (!BN_mod_lshift_quick(n2, n2, 2, p)) goto err;
	/* n2 = 4 * X_a * Y_a^2 */

	/* X_r */
	if (!BN_mod_lshift1_quick(n0, n2, p)) goto err;
	if (!field_sqr(group, &r->X, n1, ctx)) goto err;
	if (!BN_mod_sub_quick(&r->X, &r->X, n0, p)) goto err;
	/* X_r = n1^2 - 2 * n2 */
	
	/* n3 */
	if (!field_sqr(group, n0, n3, ctx)) goto err;
	if (!BN_mod_lshift_quick(n3, n0, 3, p)) goto err;
	/* n3 = 8 * Y_a^4 */
	
	/* Y_r */
	if (!BN_mod_sub_quick(n0, n2, &r->X, p)) goto err;
	if (!field_mul(group, n0, n1, n0, ctx)) goto err;
	if (!BN_mod_sub_quick(&r->Y, n0, n3, p)) goto err;
	/* Y_r = n1 * (n2 - X_r) - n3 */

	ret = 1;

 err:
	BN_CTX_end(ctx);
	if (new_ctx != NULL)
		BN_CTX_free(new_ctx);
	return ret;
	}


int ec_GFp_simple_is_at_infinity(const EC_GROUP *group, const EC_POINT *point)
	{
	return BN_is_zero(&point->Z);
	}


int ec_GFp_simple_is_on_curve(const EC_GROUP *group, const EC_POINT *point, BN_CTX *ctx);
/* TODO */


int ec_GFp_simple_make_affine(const EC_GROUP *group, const EC_POINT *point, BN_CTX *ctx);
/* TODO */


int ec_GFp_simple_field_mul(const EC_GROUP *group, BIGNUM *r, const BIGNUM *a, const BIGNUM *b, BN_CTX *ctx)
	{
	return BN_mod_mul(r, a, b, &group->field, ctx);
	}


int ec_GFp_simple_field_sqr(const EC_GROUP *group, BIGNUM *r, const BIGNUM *a, BN_CTX *ctx)
	{
	return BN_mod_sqr(r, a, &group->field, ctx);
	}
