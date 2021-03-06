/*
 * NSS certificate loading routines, for libreswan
 *
 * Copyright (C) 2015 Matt Rogers <mrogers@libreswan.org>
 * Copyright (C) 2016, Andrew Cagney <cagney@gnu.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <https://www.gnu.org/licenses/gpl2.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include "lswnss.h"
#include "lswlog.h"

#include "nss_cert_load.h"

CERTCertificate *get_cert_by_nickname_from_nss(const char *nickname)
{
	return nickname == NULL ? NULL :
		PK11_FindCertFromNickname(nickname,
			lsw_return_nss_password_file_info());
}

struct ckaid_match_arg {
	SECItem ckaid;
	CERTCertificate *cert;
};

static SECStatus ckaid_match(CERTCertificate *cert, SECItem *ignore1 UNUSED, void *arg)
{
	struct ckaid_match_arg *ckaid_match_arg = arg;
	if (ckaid_match_arg->cert != NULL) {
		return SECSuccess;
	}
	SECItem *ckaid = PK11_GetLowLevelKeyIDForCert(NULL, cert,
						      lsw_return_nss_password_file_info());
	if (ckaid == NULL) {
		DBG(DBG_CONTROL,
		    DBG_log("GetLowLevelID for cert %s failed", cert->nickname));
		return SECSuccess;
	}
	if (SECITEM_ItemsAreEqual(ckaid, &ckaid_match_arg->ckaid)) {
		DBG(DBG_CONTROLMORE, DBG_log("CKAID matched cert %s", cert->nickname));
		ckaid_match_arg->cert = CERT_DupCertificate(cert);
		/* bail early, but how?  */
	}
	SECITEM_FreeItem(ckaid, PR_TRUE);
	return SECSuccess;
}

CERTCertificate *get_cert_by_ckaid_t_from_nss(const ckaid_t *ckaid)
{
	struct ckaid_match_arg ckaid_match_arg = {
		.cert = NULL,
		.ckaid = same_ckaid_as_secitem(ckaid),
	};
	PK11_TraverseSlotCerts(ckaid_match, &ckaid_match_arg,
			       lsw_return_nss_password_file_info());
	return ckaid_match_arg.cert;
}

CERTCertificate *get_cert_by_ckaid_from_nss(const char *ckaid)
{
	if (ckaid == NULL) {
		return NULL;
	}
	/* convert hex string ckaid to binary bin */
	size_t binlen = (strlen(ckaid) + 1) / 2;
	uint8_t *bin = alloc_bytes(binlen, "ckaid");
	/* binlen will be "fixed"; ttodata doesn't take void* */
	const char *ugh = ttodata(ckaid, 0, 16, (void*)bin, binlen, &binlen);
	if (ugh != NULL) {
		pfree(bin);
		/* should have been rejected by whack? */
		libreswan_log("invalid hex CKAID '%s': %s", ckaid, ugh);
		return NULL;
	}

	/* copy blob into ckaid */
	struct ckaid_match_arg ckaid_match_arg = {
		.cert = NULL,
		.ckaid = {
			.data = bin,
			.len = binlen,
			.type = siBuffer,
		},
	};
	PK11_TraverseSlotCerts(ckaid_match, &ckaid_match_arg,
			       lsw_return_nss_password_file_info());
	pfree(bin);
	return ckaid_match_arg.cert;
}
