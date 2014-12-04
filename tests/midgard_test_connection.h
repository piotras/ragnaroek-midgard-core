
#ifndef MIDGARD_TEST_CONNECTION_H
#define MIDGARD_TEST_CONNECTION_H

#include "midgard_test_config.h"

MidgardConnection *midgard_test_connection_open_user_config(const gchar *name, MidgardConfig **config);

void midgard_test_connection_run(void);

#endif /* MIDGARD_TEST_CONNECTION_H */
