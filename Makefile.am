## Process this file with automake to produce Makefile.in

lib_LTLIBRARIES = \
	src/libmidgard.la

bin_PROGRAMS = \
	test/midgard-pageresolve \
	test/midgard-snippet \
	test/midgard-owner \
	test/midgard-preparse \
	test/midgard-schema \
	test/midgard-query

noinst_PROGRAMS = @CHECK_TESTS@
EXTRA_PROGRAMS = $(TESTS)

nobase_dist_sysconf_DATA = \
        conf.d/midgard.conf.example

dist_pkgdata_DATA = MgdObjects.xml \
	conf.d/midgard.conf.example 

nobase_pkgdata_DATA = schema/README.txt

nobase_include_HEADERS = \
	midgard/midgard_defs.h \
	midgard/midgard_config_auto.h \
	midgard/midgard_apache.h \
	midgard/select_db.h \
	midgard/select_db2.h \
	midgard/tablenames.h \
	midgard/midgard.h \
	midgard/authfailure.h \
	midgard/pageresolve.h \
	midgard/midgard_legacy.h \
	midgard/parser.h \
	midgard/pool.h \
	midgard/types.h \
	midgard/query_builder.h \
	midgard/midgard_metadata.h \
	midgard/query.h \
	midgard/midgard_timestamp.h \
	midgard/uuid.h \
	midgard/guid.h \
	midgard/midgard_datatypes.h \
	midgard/midgard_object.h \
	midgard/midgard_type.h \
	midgard/midgard_quota.h \
	midgard/types_export.h \
	midgard/midgard_config.h \
	midgard/midgard_reflection_property.h \
	midgard/midgard_object_property.h \
	midgard/midgard_schema.h \
	midgard/midgard_connection.h \
	midgard/midgard_error.h \
	midgard/midgard_collector.h \
	midgard/midgard_object_parameter.h \
	midgard/midgard_object_attachment.h \
	midgard/midgard_replicator.h \
	midgard/midgard_blob.h \
	midgard/midgard_dbobject.h \
	midgard/midgard_user.h \
	midgard/midgard_dbus.h \
	midgard/midgard_sitegroup.h

man_MANS = man/midgard-schema.1 \
	man/midgard-pageresolve.1 \
	man/midgard-snippet.1 \
	man/midgard-query.1 

EXTRA_DIST = $(man_MANS) \
	midgard.pc.in	\
	schema/README.txt

LDADD = src/libmidgard.la

$(PROGRAMS): $(LDADD)

src_libmidgard_la_CPPFLAGS = \
	-I$(top_srcdir)/src -DG_LOG_DOMAIN=\"midgard-core\"

AM_CFLAGS = -Wall @MIDGARD_CFLAGS@ @MYSQL_CFLAGS@ 
AM_LDFLAGS = @MIDGARD_LIBS@ @MYSQL_LIBS@

src_libmidgard_la_LDFLAGS = -version-info 2017:0:9 @MIDGARD_LIBS@ @MYSQL_LIBS@

test_midgard_pageresolve_SOURCES = \
	test/midgard-pageresolve.c

test_midgard_preparse_SOURCES = \
	test/midgard-preparse.c

test_midgard_snippet_SOURCES = \
	test/midgard-snippet.c

test_midgard_owner_SOURCES = \
	test/midgard-owner.c

test_midgard_schema_SOURCES = \
	test/midgard-schema.c

test_midgard_query_SOURCES = \
	test/midgard-query.c

test_midgard_schema_CPPFLAGS = \
	 -DG_LOG_DOMAIN=\"midgard-core\"

src_libmidgard_la_SOURCES = \
	src/midgard.c \
	src/format.c \
	src/fmt_russian.c \
	src/md5.c \
	src/uuid.c \
	src/guid.c \
	src/tree.c \
	src/repligard.c \
	src/access.c \
	src/tree_core.c \
	src/pageresolve.c \
	src/preparse.l \
        src/midgard_type.c \
	src/types.c \
	src/mgdschema.c \
	src/midgard_config.c \
	src/query_builder.c \
	src/query_constraint.c \
	src/simple_constraint.c \
	src/group_constraint.c \
	src/query_order.c \
	src/authfailure.c \
	src/tablenames-int.c \
	src/tablenames-ext.c \
	src/types_export.c \
	src/types_metadata.c \
	src/pool.c \
	src/query.c \
	src/midgard_object_property.c \
	src/midgard_timestamp.c \
	src/midgard_datatypes.c \
	src/midgard_object.c \
	src/midgard_quota.c \
	src/fmt_russian.h \
	src/defaults.h \
	src/md5.h \
	src/query_constraint.h \
	src/query_group_constraint.h \
	src/query_group_constraint.c \
	src/simple_constraint.h \
	src/group_constraint.h \
	src/query_order.h \
	src/tablenames-int.tmpl \
	src/tablenames-ext.tmpl \
	src/authfailure.tmpl \
	src/schema.h \
	src/midgard_mysql.h \
	src/midgard_reflection_property.c \
	src/midgard_connection.c \
	src/midgard_error.c \
	src/midgard_collector.c \
	src/midgard_core_object_class.c \
	src/midgard_core_object_class.h \
	src/midgard_core_query_builder.h \
	src/midgard_core_query_builder.c \
	src/midgard_core_object.c \
	src/midgard_core_object_parameter.h \
	src/midgard_core_object.h \
	src/midgard_core_config.c \
	src/midgard_core_config.h \
	src/midgard_core_query.h \
	src/midgard_dbus.c \
	src/midgard_dbus_interface.h \
	src/midgard_object_attachment.c \
	src/midgard_object_parameter.c \
	src/midgard_replicator.c \
	src/midgard_blob.c \
	src/midgard_dbobject.c \
	src/gbase64.c \
	src/gbase64.h \
	src/midgard_user.c \
	src/midgard_sitegroup.c
	
src/authfailure.c: src/authfailure.tmpl midgard/authfailure.h
	glib-mkenums --template $^ > $@

src/tablenames-int.c: src/tablenames-int.tmpl midgard/tablenames.h
	glib-mkenums --template $^ > $@

src/tablenames-ext.c: src/tablenames-ext.tmpl midgard/tablenames.h
	glib-mkenums --template $^ > $@

CLEANFILES = \
	preparse.c \
	src/authfailure.c \
	src/tablenames-int.c \
	src/tablenames-ext.c

pkgdatadir = $(datadir)/midgard
pkglibdir = $(libdir)/midgard
pkgincludedir = $(includedir)/midgard

mgdschemasdir = $(datadir)/midgard/schema

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = midgard.pc

doc:
	doxygen

.PHONY: doc
