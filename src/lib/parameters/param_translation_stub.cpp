/**
 * param_translation_stub.cpp
 *
 * Stub for param_modify_on_import while param_translation.cpp is not yet
 * compiled (it depends on Device.hpp / drv_sensor.h not yet ported).
 *
 * TODO: 后期移除此文件，取消 CMakeLists.txt 中 param_translation.cpp 的注释
 */

#include "param_translation.h"

param_modify_on_import_ret param_modify_on_import(bson_node_t /* node */)
{
	return param_modify_on_import_ret::PARAM_NOT_MODIFIED;
}
