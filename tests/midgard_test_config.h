
#ifndef MIDGARD_TEST_CONFIG_H
#define MIDGARD_TEST_CONFIG_H

#include <midgard/midgard.h>

#define CONFIG_CONFIG_NAME "midgard_test"
#define CONFIG_DB_NAME "midgard_test"

MidgardConfig *midgard_test_config_new_user_config(const gchar *name);
void midgard_test_config_init(void);

#endif /* MIDGARD_TEST_CONFIG_H */
