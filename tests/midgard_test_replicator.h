
#ifndef MIDGARD_TEST_REPLICATOR_H
#define MIDGARD_TEST_REPLICATOR_H

#include "midgard_test.h"
#include "midgard_test_object.h"

/* tests */
void midgard_test_replicator_new(MgdObjectTest *mot, gconstpointer data);
void midgard_test_replicator_serialize(MgdObjectTest *mot, gconstpointer data);
void midgard_test_replicator_serialize_multilang(MgdObjectTest *mot, gconstpointer data);
void midgard_test_replicator_export(MgdObjectTest *mot, gconstpointer data);
void midgard_test_replicator_export_purged(MgdObjectTest *mot, gconstpointer data);
void midgard_test_replicator_serialize_blob(MgdObjectTest *mot, gconstpointer data);
void midgard_test_replicator_export_blob(MgdObjectTest *mot, gconstpointer data);
void midgard_test_replicator_export_by_guid(MgdObjectTest *mot, gconstpointer data);
void midgard_test_replicator_export_media(MgdObjectTest *mot, gconstpointer data);
void midgard_test_replicator_unserialize(MgdObjectTest *mot, gconstpointer data);
void midgard_test_replicator_import_object(MgdObjectTest *mot, gconstpointer data);
void midgard_test_replicator_import_from_xml(MgdObjectTest *mot, gconstpointer data);
void midgard_test_replicator_xml_is_valid(MgdObjectTest *mot, gconstpointer data);

void midgard_test_replicator_update_object_links(MgdObjectTest *mot, gconstpointer data);

#endif /* MIDGARD_TEST_REPLICATOR_H */
