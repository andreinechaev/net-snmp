/*
 * keytools.h
 */

#ifndef _KEYTOOLS_H
#define _KEYTOOLS_H

#ifdef __cplusplus
extern "C" {
#endif


#define USM_LENGTH_EXPANDED_PASSPHRASE	(1024 * 1024)	/* 1Meg. */

#define USM_LENGTH_KU_HASHBLOCK		64		/* In bytes. */

#define USM_LENGTH_P_MIN		8		/* In characters. */
	/* Recommended practice given in <draft-ietf-snmpv3-usm-v2-02.txt>,
	 * Section 11.2 "Defining Users".  Move into cmdline app argument
	 * parsing, and out of the internal routine?  XXX
	 */

#define SET_HASH_TRANSFORM(t)   kmt_hash = t;



/*
 * Simple hash function pointer, and the internal hash functions offered
 * by KMT.
 *
 * FIX  Resolve the broken KMT API issue.
 * 	kmt_s_* prototypes stolen from KMT/algs/kmt_hash.h.
 *
 * FIX	Offer an snmp_hash() function to hide away differences between
 *	this an "internal" MD5 (& whatever else might come...)?
 */
extern int (*kmt_hash) (
	const int	  mode,		void  	 **context,
	const u_int8_t	 *data,		const int  data_len,     
	u_int8_t	**digest,	int	  *digest_len);


#ifdef HAVE_LIBKMT
extern int (*kmt_s_md5) (
		const int	  mode,		void  	 **context,
		const u_int8_t	 *data,		const int  data_len,     
		u_int8_t	**digest,	int	  *digest_len);
extern int (*kmt_s_sha1) (
		const int	  mode,		void  	 **context,
		const u_int8_t	 *data,		const int  data_len,     
		u_int8_t	**digest,	int	  *digest_len);
extern int (*kmt_s_ripemd) (
		const int	  mode,		void  	 **context,
		const u_int8_t	 *data,		const int  data_len,     
		u_int8_t	**digest,	int	  *digest_len);

#endif

/*
 * Prototypes.h
 */
int	generate_Ku (	oid	*hashtype,	u_int  hashtype_len,
				u_char	*P,		size_t  pplen,
				u_char	*Ku,		size_t *kulen);

int	generate_kul (	oid	*hashtype,	u_int  hashtype_len,
				u_char	*engineID,	size_t  engineID_len,
				u_char	*Ku,		size_t  ku_len,
				u_char	*Kul,		size_t *kul_len);

int	encode_keychange (	oid	*hashtype,	u_int  hashtype_len,
				u_char	*oldkey,	size_t  oldkey_len,
				u_char	*newkey,	size_t  newkey_len,
				u_char	*kcstring,	size_t *kcstring_len);

int	decode_keychange (	oid	*hashtype,	u_int  hashtype_len,
				u_char	*oldkey,	size_t  oldkey_len,
				u_char	*kcstring,	size_t  kcstring_len,
				u_char	*newkey,	size_t *newkey_len);


/*
 * All functions devolve to the following block if HAVE_LIBKMT is not defined.
 */
#define	_KEYTOOLS_NOT_AVAILABLE			\
{						\
	return SNMPERR_KT_NOT_AVAILABLE;	\
}

#ifdef __cplusplus
}
#endif

#endif /* _KEYTOOLS_H */

