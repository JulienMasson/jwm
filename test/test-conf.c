/*
 * This file is part of the jwm distribution:
 * https://github.com/JulienMasson/jwm
 *
 * Copyright (c) 2018 Julien Masson.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "core.h"
#include "conf.h"


START(conf_read_fail)
{
    fail_unless(conf_read() == -1, "No config set, should failed");
}
END(conf_read_fail);


START(conf_read_fail2)
{
    char conf_path[] = "asdasdasd";
    conf_init(conf_path);
    fail_unless(conf_read() == -1, "No config set, should failed");
}
END(conf_read_fail2);


START(conf_read_pass)
{
    char conf_path[] = ROOT_DIR"/res/.jwmrc";
    conf_init(conf_path);
    fail_unless(conf_read() == 0, "Cannot read the config");
}
END(conf_read_pass);
