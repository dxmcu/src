#	$NetBSD: dso.inc,v 1.3 2007/12/09 22:44:15 adrianp Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/dso/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/dso


DSO_SRCS = dso_dl.c dso_dlfcn.c dso_err.c dso_lib.c dso_null.c \
	dso_openssl.c dso_win32.c dso_vms.c
SRCS += ${DSO_SRCS}

.for cryptosrc in ${DSO_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/dso
.endfor