/*
 * Digital Signature Standard implementation for PuTTY.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "ssh.h"
#include "misc.h"

static void sha_mpint(SHA_State * s, Bignum b)
{
    unsigned char lenbuf[4];
    int len;
    len = (bignum_bitcount(b) + 8) / 8;
    PUT_32BIT(lenbuf, len);
    SHA_Bytes(s, lenbuf, 4);
    while (len-- > 0)
    {
        lenbuf[0] = bignum_byte(b, len);
        SHA_Bytes(s, lenbuf, 1);
    }
    smemclr(lenbuf, sizeof(lenbuf));
}

static void sha512_mpint(SHA512_State * s, Bignum b)
{
    unsigned char lenbuf[4];
    int len;
    len = (bignum_bitcount(b) + 8) / 8;
    PUT_32BIT(lenbuf, len);
    SHA512_Bytes(s, lenbuf, 4);
    while (len-- > 0)
    {
        lenbuf[0] = bignum_byte(b, len);
        SHA512_Bytes(s, lenbuf, 1);
    }
    smemclr(lenbuf, sizeof(lenbuf));
}

static void getstring(const char **data, int *datalen,
                      const char **p, int *length)
{
    *p = NULL;
    if (*datalen < 4)
        return;
    *length = toint(GET_32BIT(*data));
    if (*length < 0)
        return;
    *datalen -= 4;
    *data += 4;
    if (*datalen < *length)
        return;
    *p = *data;
    *data += *length;
    *datalen -= *length;
}
static Bignum getmp(const char **data, int *datalen)
{
    const char *p;
    int length;
    Bignum b;

    getstring(data, datalen, &p, &length);
    if (!p)
        return NULL;
    if (p[0] & 0x80)
        return NULL;		       /* negative mp */
    b = bignum_from_bytes((const unsigned char *)p, length);
    return b;
}

static Bignum get160(const char **data, int *datalen)
{
    Bignum b;

    if (*datalen < 20)
        return NULL;

    b = bignum_from_bytes((const unsigned char *)*data, 20);
    *data += 20;
    *datalen -= 20;

    return b;
}

static void dss_freekey(void *key);    /* forward reference */

static void *dss_newkey(const struct ssh_signkey *self,
                        const char *data, int len)
{
    const char *p;
    int slen;
    struct dss_key *dss;

    dss = snew(struct dss_key);
    getstring(&data, &len, &p, &slen);

#ifdef DEBUG_DSS
    {
        int i;
        printf("key:");
        for (i = 0; i < len; i++)
            printf("  %02x", (unsigned char) (data[i]));
        printf("\n");
    }
#endif

    if (!p || slen != 7 || memcmp(p, "ssh-dss", 7))
    {
        sfree(dss);
        return NULL;
    }
    dss->p = getmp(&data, &len);
    dss->q = getmp(&data, &len);
    dss->g = getmp(&data, &len);
    dss->y = getmp(&data, &len);
    dss->x = NULL;

    if (!dss->p || !dss->q || !dss->g || !dss->y ||
            !bignum_cmp(dss->q, Zero) || !bignum_cmp(dss->p, Zero))
    {
        /* Invalid key. */
        dss_freekey(dss);
        return NULL;
    }

    return dss;
}

static void dss_freekey(void *key)
{
    struct dss_key *dss = (struct dss_key *) key;
    if (dss->p)
        freebn(dss->p);
    if (dss->q)
        freebn(dss->q);
    if (dss->g)
        freebn(dss->g);
    if (dss->y)
        freebn(dss->y);
    if (dss->x)
        freebn(dss->x);
    sfree(dss);
}

static char *dss_fmtkey(void *key)
{
    struct dss_key *dss = (struct dss_key *) key;
    char *p;
    int len, i, pos, nibbles;
    static const char hex[] = "0123456789abcdef";
    if (!dss->p)
        return NULL;
    len = 8 + 4 + 1;		       /* 4 x "0x", punctuation, \0 */
    len += 4 * (bignum_bitcount(dss->p) + 15) / 16;
    len += 4 * (bignum_bitcount(dss->q) + 15) / 16;
    len += 4 * (bignum_bitcount(dss->g) + 15) / 16;
    len += 4 * (bignum_bitcount(dss->y) + 15) / 16;
    p = snewn(len, char);
    if (!p)
        return NULL;

    pos = 0;
    pos += sprintf(p + pos, "0x");
    nibbles = (3 + bignum_bitcount(dss->p)) / 4;
    if (nibbles < 1)
        nibbles = 1;
    for (i = nibbles; i--;)
        p[pos++] =
            hex[(bignum_byte(dss->p, i / 2) >> (4 * (i % 2))) & 0xF];
    pos += sprintf(p + pos, ",0x");
    nibbles = (3 + bignum_bitcount(dss->q)) / 4;
    if (nibbles < 1)
        nibbles = 1;
    for (i = nibbles; i--;)
        p[pos++] =
            hex[(bignum_byte(dss->q, i / 2) >> (4 * (i % 2))) & 0xF];
    pos += sprintf(p + pos, ",0x");
    nibbles = (3 + bignum_bitcount(dss->g)) / 4;
    if (nibbles < 1)
        nibbles = 1;
    for (i = nibbles; i--;)
        p[pos++] =
            hex[(bignum_byte(dss->g, i / 2) >> (4 * (i % 2))) & 0xF];
    pos += sprintf(p + pos, ",0x");
    nibbles = (3 + bignum_bitcount(dss->y)) / 4;
    if (nibbles < 1)
        nibbles = 1;
    for (i = nibbles; i--;)
        p[pos++] =
            hex[(bignum_byte(dss->y, i / 2) >> (4 * (i % 2))) & 0xF];
    p[pos] = '\0';
    return p;
}

static int dss_verifysig(void *key, const char *sig, int siglen,
                         const char *data, int datalen)
{
    struct dss_key *dss = (struct dss_key *) key;
    const char *p;
    int slen;
    char hash[20];
    Bignum r, s, w, gu1p, yu2p, gu1yu2p, u1, u2, sha, v;
    int ret;

    if (!dss->p)
        return 0;

#ifdef DEBUG_DSS
    {
        int i;
        printf("sig:");
        for (i = 0; i < siglen; i++)
            printf("  %02x", (unsigned char) (sig[i]));
        printf("\n");
    }
#endif
    /*
     * Commercial SSH (2.0.13) and OpenSSH disagree over the format
     * of a DSA signature. OpenSSH is in line with RFC 4253:
     * it uses a string "ssh-dss", followed by a 40-byte string
     * containing two 160-bit integers end-to-end. Commercial SSH
     * can't be bothered with the header bit, and considers a DSA
     * signature blob to be _just_ the 40-byte string containing
     * the two 160-bit integers. We tell them apart by measuring
     * the length: length 40 means the commercial-SSH bug, anything
     * else is assumed to be RFC-compliant.
     */
    if (siglen != 40)  		       /* bug not present; read admin fields */
    {
        getstring(&sig, &siglen, &p, &slen);
        if (!p || slen != 7 || memcmp(p, "ssh-dss", 7))
        {
            return 0;
        }
        sig += 4, siglen -= 4;	       /* skip yet another length field */
    }
    r = get160(&sig, &siglen);
    s = get160(&sig, &siglen);
    if (!r || !s)
    {
        if (r)
            freebn(r);
        if (s)
            freebn(s);
        return 0;
    }

    if (!bignum_cmp(s, Zero))
    {
        freebn(r);
        freebn(s);
        return 0;
    }

    /*
     * Step 1. w <- s^-1 mod q.
     */
    w = modinv(s, dss->q);
    if (!w)
    {
        freebn(r);
        freebn(s);
        return 0;
    }

    /*
     * Step 2. u1 <- SHA(message) * w mod q.
     */
    SHA_Simple(data, datalen, (unsigned char *)hash);
    p = hash;
    slen = 20;
    sha = get160(&p, &slen);
    u1 = modmul(sha, w, dss->q);

    /*
     * Step 3. u2 <- r * w mod q.
     */
    u2 = modmul(r, w, dss->q);

    /*
     * Step 4. v <- (g^u1 * y^u2 mod p) mod q.
     */
    gu1p = modpow(dss->g, u1, dss->p);
    yu2p = modpow(dss->y, u2, dss->p);
    gu1yu2p = modmul(gu1p, yu2p, dss->p);
    v = modmul(gu1yu2p, One, dss->q);

    /*
     * Step 5. v should now be equal to r.
     */

    ret = !bignum_cmp(v, r);

    freebn(w);
    freebn(sha);
    freebn(u1);
    freebn(u2);
    freebn(gu1p);
    freebn(yu2p);
    freebn(gu1yu2p);
    freebn(v);
    freebn(r);
    freebn(s);

    return ret;
}

static unsigned char *dss_public_blob(void *key, int *len)
{
    struct dss_key *dss = (struct dss_key *) key;
    int plen, qlen, glen, ylen, bloblen;
    int i;
    unsigned char *blob, *p;

    plen = (bignum_bitcount(dss->p) + 8) / 8;
    qlen = (bignum_bitcount(dss->q) + 8) / 8;
    glen = (bignum_bitcount(dss->g) + 8) / 8;
    ylen = (bignum_bitcount(dss->y) + 8) / 8;

    /*
     * string "ssh-dss", mpint p, mpint q, mpint g, mpint y. Total
     * 27 + sum of lengths. (five length fields, 20+7=27).
     */
    bloblen = 27 + plen + qlen + glen + ylen;
    blob = snewn(bloblen, unsigned char);
    p = blob;
    PUT_32BIT(p, 7);
    p += 4;
    memcpy(p, "ssh-dss", 7);
    p += 7;
    PUT_32BIT(p, plen);
    p += 4;
    for (i = plen; i--;)
        *p++ = bignum_byte(dss->p, i);
    PUT_32BIT(p, qlen);
    p += 4;
    for (i = qlen; i--;)
        *p++ = bignum_byte(dss->q, i);
    PUT_32BIT(p, glen);
    p += 4;
    for (i = glen; i--;)
        *p++ = bignum_byte(dss->g, i);
    PUT_32BIT(p, ylen);
    p += 4;
    for (i = ylen; i--;)
        *p++ = bignum_byte(dss->y, i);
    assert(p == blob + bloblen);
    *len = bloblen;
    return blob;
}

static unsigned char *dss_private_blob(void *key, int *len)
{
    struct dss_key *dss = (struct dss_key *) key;
    int xlen, bloblen;
    int i;
    unsigned char *blob, *p;

    xlen = (bignum_bitcount(dss->x) + 8) / 8;

    /*
     * mpint x, string[20] the SHA of p||q||g. Total 4 + xlen.
     */
    bloblen = 4 + xlen;
    blob = snewn(bloblen, unsigned char);
    p = blob;
    PUT_32BIT(p, xlen);
    p += 4;
    for (i = xlen; i--;)
        *p++ = bignum_byte(dss->x, i);
    assert(p == blob + bloblen);
    *len = bloblen;
    return blob;
}

static void *dss_createkey(const struct ssh_signkey *self,
                           const unsigned char *pub_blob, int pub_len,
                           const unsigned char *priv_blob, int priv_len)
{
    struct dss_key *dss;
    const char *pb = (const char *) priv_blob;
    const char *hash;
    int hashlen;
    SHA_State s;
    unsigned char digest[20];
    Bignum ytest;

    dss = (struct dss_key *)dss_newkey(self, (char *) pub_blob, pub_len);
    if (!dss)
        return NULL;
    dss->x = getmp(&pb, &priv_len);
    if (!dss->x)
    {
        dss_freekey(dss);
        return NULL;
    }

    /*
     * Check the obsolete hash in the old DSS key format.
     */
    hashlen = -1;
    getstring(&pb, &priv_len, &hash, &hashlen);
    if (hashlen == 20)
    {
        SHA_Init(&s);
        sha_mpint(&s, dss->p);
        sha_mpint(&s, dss->q);
        sha_mpint(&s, dss->g);
        SHA_Final(&s, digest);
        if (0 != memcmp(hash, digest, 20))
        {
            dss_freekey(dss);
            return NULL;
        }
    }

    /*
     * Now ensure g^x mod p really is y.
     */
    ytest = modpow(dss->g, dss->x, dss->p);
    if (0 != bignum_cmp(ytest, dss->y))
    {
        dss_freekey(dss);
        freebn(ytest);
        return NULL;
    }
    freebn(ytest);

    return dss;
}

static void *dss_openssh_createkey(const struct ssh_signkey *self,
                                   const unsigned char **blob, int *len)
{
    const char **b = (const char **) blob;
    struct dss_key *dss;

    dss = snew(struct dss_key);

    dss->p = getmp(b, len);
    dss->q = getmp(b, len);
    dss->g = getmp(b, len);
    dss->y = getmp(b, len);
    dss->x = getmp(b, len);

    if (!dss->p || !dss->q || !dss->g || !dss->y || !dss->x ||
            !bignum_cmp(dss->q, Zero) || !bignum_cmp(dss->p, Zero))
    {
        /* Invalid key. */
        dss_freekey(dss);
        return NULL;
    }

    return dss;
}

static int dss_openssh_fmtkey(void *key, unsigned char *blob, int len)
{
    struct dss_key *dss = (struct dss_key *) key;
    int bloblen, i;

    bloblen =
        ssh2_bignum_length(dss->p) +
        ssh2_bignum_length(dss->q) +
        ssh2_bignum_length(dss->g) +
        ssh2_bignum_length(dss->y) +
        ssh2_bignum_length(dss->x);

    if (bloblen > len)
        return bloblen;

    bloblen = 0;
#define ENC(x) \
    PUT_32BIT(blob+bloblen, ssh2_bignum_length((x))-4); bloblen += 4; \
    for (i = ssh2_bignum_length((x))-4; i-- ;) blob[bloblen++]=bignum_byte((x),i);
    ENC(dss->p);
    ENC(dss->q);
    ENC(dss->g);
    ENC(dss->y);
    ENC(dss->x);

    return bloblen;
}

static int dss_pubkey_bits(const struct ssh_signkey *self,
                           const void *blob, int len)
{
    struct dss_key *dss;
    int ret;

    dss = (struct dss_key *)dss_newkey(self, (const char *) blob, len);
    if (!dss)
        return -1;
    ret = bignum_bitcount(dss->p);
    dss_freekey(dss);

    return ret;
}

Bignum dss_gen_k(const char *id_string, Bignum modulus, Bignum private_key,
                 unsigned char *digest, int digest_len)
{
    /*
     * The basic DSS signing algorithm is:
     *
     *  - invent a random k between 1 and q-1 (exclusive).
     *  - Compute r = (g^k mod p) mod q.
     *  - Compute s = k^-1 * (hash + x*r) mod q.
     *
     * This has the dangerous properties that:
     *
     *  - if an attacker in possession of the public key _and_ the
     *    signature (for example, the host you just authenticated
     *    to) can guess your k, he can reverse the computation of s
     *    and work out x = r^-1 * (s*k - hash) mod q. That is, he
     *    can deduce the private half of your key, and masquerade
     *    as you for as long as the key is still valid.
     *
     *  - since r is a function purely of k and the public key, if
     *    the attacker only has a _range of possibilities_ for k
     *    it's easy for him to work through them all and check each
     *    one against r; he'll never be unsure of whether he's got
     *    the right one.
     *
     *  - if you ever sign two different hashes with the same k, it
     *    will be immediately obvious because the two signatures
     *    will have the same r, and moreover an attacker in
     *    possession of both signatures (and the public key of
     *    course) can compute k = (hash1-hash2) * (s1-s2)^-1 mod q,
     *    and from there deduce x as before.
     *
     *  - the Bleichenbacher attack on DSA makes use of methods of
     *    generating k which are significantly non-uniformly
     *    distributed; in particular, generating a 160-bit random
     *    number and reducing it mod q is right out.
     *
     * For this reason we must be pretty careful about how we
     * generate our k. Since this code runs on Windows, with no
     * particularly good system entropy sources, we can't trust our
     * RNG itself to produce properly unpredictable data. Hence, we
     * use a totally different scheme instead.
     *
     * What we do is to take a SHA-512 (_big_) hash of the private
     * key x, and then feed this into another SHA-512 hash that
     * also includes the message hash being signed. That is:
     *
     *   proto_k = SHA512 ( SHA512(x) || SHA160(message) )
     *
     * This number is 512 bits long, so reducing it mod q won't be
     * noticeably non-uniform. So
     *
     *   k = proto_k mod q
     *
     * This has the interesting property that it's _deterministic_:
     * signing the same hash twice with the same key yields the
     * same signature.
     *
     * Despite this determinism, it's still not predictable to an
     * attacker, because in order to repeat the SHA-512
     * construction that created it, the attacker would have to
     * know the private key value x - and by assumption he doesn't,
     * because if he knew that he wouldn't be attacking k!
     *
     * (This trick doesn't, _per se_, protect against reuse of k.
     * Reuse of k is left to chance; all it does is prevent
     * _excessively high_ chances of reuse of k due to entropy
     * problems.)
     *
     * Thanks to Colin Plumb for the general idea of using x to
     * ensure k is hard to guess, and to the Cambridge University
     * Computer Security Group for helping to argue out all the
     * fine details.
     */
    SHA512_State ss;
    unsigned char digest512[64];
    Bignum proto_k, k;

    /*
     * Hash some identifying text plus x.
     */
    SHA512_Init(&ss);
    SHA512_Bytes(&ss, id_string, strlen(id_string) + 1);
    sha512_mpint(&ss, private_key);
    SHA512_Final(&ss, digest512);

    /*
     * Now hash that digest plus the message hash.
     */
    SHA512_Init(&ss);
    SHA512_Bytes(&ss, digest512, sizeof(digest512));
    SHA512_Bytes(&ss, digest, digest_len);

    while (1)
    {
        SHA512_State ss2 = ss;         /* structure copy */
        SHA512_Final(&ss2, digest512);

        smemclr(&ss2, sizeof(ss2));

        /*
         * Now convert the result into a bignum, and reduce it mod q.
         */
        proto_k = bignum_from_bytes(digest512, 64);
        k = bigmod(proto_k, modulus);
        freebn(proto_k);

        if (bignum_cmp(k, One) != 0 && bignum_cmp(k, Zero) != 0)
        {
            smemclr(&ss, sizeof(ss));
            smemclr(digest512, sizeof(digest512));
            return k;
        }

        /* Very unlikely we get here, but if so, k was unsuitable. */
        freebn(k);
        /* Perturb the hash to think of a different k. */
        SHA512_Bytes(&ss, "x", 1);
        /* Go round and try again. */
    }
}

static unsigned char *dss_sign(void *key, const char *data, int datalen,
                               int *siglen)
{
    struct dss_key *dss = (struct dss_key *) key;
    Bignum k, gkp, hash, kinv, hxr, r, s;
    unsigned char digest[20];
    unsigned char *bytes;
    int nbytes, i;

    SHA_Simple(data, datalen, digest);

    k = dss_gen_k("DSA deterministic k generator", dss->q, dss->x,
                  digest, sizeof(digest));
    kinv = modinv(k, dss->q);	       /* k^-1 mod q */
    assert(kinv);

    /*
     * Now we have k, so just go ahead and compute the signature.
     */
    gkp = modpow(dss->g, k, dss->p);   /* g^k mod p */
    r = bigmod(gkp, dss->q);	       /* r = (g^k mod p) mod q */
    freebn(gkp);

    hash = bignum_from_bytes(digest, 20);
    hxr = bigmuladd(dss->x, r, hash);  /* hash + x*r */
    s = modmul(kinv, hxr, dss->q);     /* s = k^-1 * (hash + x*r) mod q */
    freebn(hxr);
    freebn(kinv);
    freebn(k);
    freebn(hash);

    /*
     * Signature blob is
     *
     *   string  "ssh-dss"
     *   string  two 20-byte numbers r and s, end to end
     *
     * i.e. 4+7 + 4+40 bytes.
     */
    nbytes = 4 + 7 + 4 + 40;
    bytes = snewn(nbytes, unsigned char);
    PUT_32BIT(bytes, 7);
    memcpy(bytes + 4, "ssh-dss", 7);
    PUT_32BIT(bytes + 4 + 7, 40);
    for (i = 0; i < 20; i++)
    {
        bytes[4 + 7 + 4 + i] = bignum_byte(r, 19 - i);
        bytes[4 + 7 + 4 + 20 + i] = bignum_byte(s, 19 - i);
    }
    freebn(r);
    freebn(s);

    *siglen = nbytes;
    return bytes;
}

const struct ssh_signkey ssh_dss =
{
    dss_newkey,
    dss_freekey,
    dss_fmtkey,
    dss_public_blob,
    dss_private_blob,
    dss_createkey,
    dss_openssh_createkey,
    dss_openssh_fmtkey,
    5 /* p,q,g,y,x */,
    dss_pubkey_bits,
    dss_verifysig,
    dss_sign,
    "ssh-dss",
    "dss",
    NULL,
};
