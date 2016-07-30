/*************************************************************************/
/*  sdk.cpp                                                              */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                    http://www.godotengine.org                         */
/*************************************************************************/
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                 */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/
#ifdef MODULE_SDK_ENABLED

#include "sdk.h"

static const char *CALLBACK_INIT = "init";
static const char *CALLBACK_LOGIN = "login";
static const char *CALLBACK_SWITCH_LOGIN = "switch_login";
static const char *CALLBACK_LOGOUT = "logout";
static const char *CALLBACK_PAY = "pay";
static const char *CALLBACK_RESULT = "result";

enum U8Codes {
	CODE_NO_NETWORK = 0,			// 没有网络连接
	CODE_INIT_SUCCESS = 1,			// 初始化成功
	CODE_INIT_FAIL = 2,				// 初始化失败
	CODE_UNINIT = 3,				// 没有初始化
	CODE_LOGIN_SUCCESS = 4,			// 登录成功
	CODE_LOGIN_FAIL = 5,			// 登录失败
	CODE_LOGIN_TIMEOUT = 6,			// 登录超时
	CODE_UNLOGIN = 7,				// 没有登录
	CODE_LOGOUT_SUCCESS = 8,		// 登出成功
	CODE_LOGOUT_FAIL = 9,			// 登出失败
	CODE_PAY_SUCCESS = 10,			// 支付成功
	CODE_PAY_FAIL = 11,				// 支付失败
	CODE_TAG_ADD_SUC = 12,			// 添加Tag成功
	CODE_TAG_ADD_FAIL = 13,			// 添加Tag失败
	CODE_TAG_DEL_SUC = 14,			// 删除Tag成功
	CODE_TAG_DEL_FAIL = 15,			// 删除Tag失败
	CODE_ALIAS_ADD_SUC = 16,		// 添加Alias成功
	CODE_ALIAS_ADD_FAIL = 17,		// 添加Alias失败
	CODE_ALIAS_REMOVE_SUC = 18,		// 删除Alias成功
	CODE_ALIAS_REMOVE_FAIL = 19,	// 删除Alias失败
	CODE_PUSH_MSG_RECIEVED = 20,	// Push 收到msg
	CODE_PARAM_ERROR = 21,			// 参数 错误
	CODE_PARAM_NOT_COMPLETE = 22,	// 参数不全
	CODE_SHARE_SUCCESS = 23,		// 分享成功
	CODE_SHARE_FAILED = 24,			// 分享失败
	CODE_UPDATE_SUCCESS = 25,		// 更新成功
	CODE_UPDATE_FAILED = 26,		// 更新失败
	CODE_REAL_NAME_REG_SUC = 27,	// 实名注册成功
	CODE_REAL_NAME_REG_FAILED = 28,	// 实名注册失败
	CODE_ADDICTION_ANTI_RESULT = 29,// 房沉迷查询结果
	CODE_PUSH_ENABLED = 30,			// 推送enable成功的回调，携带一个参数，比如友盟推送，这参数是Device Token
	CODE_POST_GIFT_SUC = 31,		// 提交礼包兑换码成功
	CODE_POST_GIFT_FAILED = 32,		// 提交礼包兑换码失败
};
static const char *U8CodeStrings[] = {
	"no_network",
	"init_success",
	"init_fail",
	"uninit",
	"login_success",
	"login_fail",
	"login_timeout",
	"unlogin",
	"logout_success",
	"logout_fail",
	"pay_success",
	"pay_fail",
	"tag_add_suc",
	"tag_add_fail",
	"tag_del_suc",
	"tag_del_fail",
	"alias_add_suc",
	"alias_add_fail",
	"alias_remove_suc",
	"alias_remove_fail",
	"push_msg_recieved",
	"param_error",
	"param_not_complete",
	"share_success",
	"share_failed",
	"update_success",
	"upate_failed",
	"real_name_reg_suc",
	"real_name_reg_failed",
	"addiction_anti_result",
	"push_enabled",
	"post_gift_suc",
	"post_gift_failed",
};

Sdk *Sdk::instance = NULL;

void Sdk::on_result(int p_code, const String& p_msg) {

	Dictionary d;
	d["code"] = p_code;
	d["what"] = U8CodeStrings[p_code];
	d["msg"] = p_msg;

	switch(p_code) {
	case CODE_INIT_SUCCESS:
		sendCallback(CALLBACK_INIT, d);
		return;
	case CODE_INIT_FAIL:
		tip("SDK初始化失败");
		break;
	case CODE_LOGIN_FAIL:
		// 这里不需要提示，一般SDK有提示
		// sdk.tip("SDK登录失败");
		break;
   	case CODE_SHARE_SUCCESS:
    	tip("分享成功");
    	break;
    case CODE_SHARE_FAILED:
    	tip("分享失败");
    	break;
	case CODE_PAY_FAIL:
		tip("支付失败");
		break;
	case CODE_PAY_SUCCESS:
		// 一般这里不用提示
		// context.tip("支付成功,到账时间可能稍有延迟");
 		break;
	}
	sendCallback(CALLBACK_RESULT, d);
}

void Sdk::sendCallback(const String p_what, const Dictionary& p_data) {

	Object *obj = ObjectDB::get_instance(handler);
	if(obj == NULL)
		return;
	obj->call(callback, p_what, p_data);
}

void Sdk::init(int p_handler, const String& p_callback) {

	this->handler = p_handler;
	this->callback = p_callback;

	on_result(CODE_INIT_SUCCESS, "SDK initialize success");
}

bool Sdk::is_support(const String& p_plugin, const String& p_what) {

	if(p_plugin == "user") {
		if(p_what == "exit")
			return false;
	}
	return false;
}

int Sdk::get_curr_channel() {
	return 0;
}

int Sdk::get_logic_channel() {
	return 0;
}

int Sdk::get_app_id() {
	return -1;
}

String Sdk::get_app_key() {
	return "";
}

void Sdk::tip(const String& p_tip) {

	printf("SDK: tip '%s'\n", p_tip.utf8().get_data());
}

void Sdk::login() {

	login_custom("");
}

void Sdk::login_custom(const String& p_extension) {

	Dictionary d;
	d["success"] = true;
	d["switch_account"] = false;
	d["extension"] = p_extension;
	d["user_id"] = -1;
	d["user_name"] = "";
	d["sdk_user_id"] = "";
	d["sdk_user_name"] = "";
	d["token"] = "";
	on_result(CODE_LOGIN_SUCCESS, "login success");
	sendCallback(CALLBACK_SWITCH_LOGIN);
	sendCallback(CALLBACK_LOGIN, d);
}

void Sdk::switch_login() {

	Dictionary d;
	d["success"] = true;
	d["switch_account"] = true;
	d["extension"] = "";
	d["user_id"] = -1;
	d["user_name"] = "";
	d["sdk_user_id"] = "";
	d["sdk_user_name"] = "";
	d["token"] = "";
	on_result(CODE_LOGIN_SUCCESS, "login success");
	sendCallback(CALLBACK_LOGIN, d);
}

void Sdk::logout() {

	on_result(CODE_LOGOUT_SUCCESS, "logout success");
	sendCallback(CALLBACK_LOGOUT);
}

void Sdk::show_user_center() {
}

void Sdk::submit_extra(const Dictionary& p_data) {

	p_data;
}

void Sdk::exit() {
}

void Sdk::pay(const Dictionary& p_data) {

	p_data;
}

void Sdk::start_push() {
}

void Sdk::stop_push() {
}

void Sdk::add_tags(const StringArray& p_tags) {

	p_tags;
}

void Sdk::remove_tags(const StringArray& p_tags) {

	p_tags;
}

void Sdk::add_alias(const String& p_alias) {

	p_alias;
}

void Sdk::remove_alias(const String& p_alias) {

	p_alias;
}

void Sdk::share(const Dictionary& p_data) {

	p_data;
}

void Sdk::analytics(const Dictionary& p_data) {

	p_data;
}

void Sdk::download(const String& p_url, bool p_show_confirm, bool p_force) {

	p_url; p_show_confirm; p_force;
}

void Sdk::_bind_methods() {

 	ObjectTypeDB::bind_method(_MD("init", "inst_id", "callback"),&Sdk::init);
 	ObjectTypeDB::bind_method(_MD("is_support", "plugin_name", "what"),&Sdk::is_support);
 	ObjectTypeDB::bind_method(_MD("tip", "tip"),&Sdk::tip);
 	ObjectTypeDB::bind_method(_MD("login"),&Sdk::login);
 	ObjectTypeDB::bind_method(_MD("login_custom", "extension"),&Sdk::login_custom);
 	ObjectTypeDB::bind_method(_MD("switch_login"),&Sdk::switch_login);
 	ObjectTypeDB::bind_method(_MD("logout"),&Sdk::logout);
 	ObjectTypeDB::bind_method(_MD("show_user_center"),&Sdk::show_user_center);
 	ObjectTypeDB::bind_method(_MD("submit_extra", "extra_data"),&Sdk::submit_extra);
 	ObjectTypeDB::bind_method(_MD("exit"),&Sdk::exit);
 	ObjectTypeDB::bind_method(_MD("pay", "pay_params"),&Sdk::pay);
 	ObjectTypeDB::bind_method(_MD("start_push"),&Sdk::start_push);
 	ObjectTypeDB::bind_method(_MD("stop_push"),&Sdk::stop_push);
 	ObjectTypeDB::bind_method(_MD("add_tags", "tags"),&Sdk::add_tags);
 	ObjectTypeDB::bind_method(_MD("remove_tags", "tags"),&Sdk::remove_tags);
 	ObjectTypeDB::bind_method(_MD("add_alias", "alias"),&Sdk::add_alias);
 	ObjectTypeDB::bind_method(_MD("remove_alias", "alias"),&Sdk::remove_alias);
 	ObjectTypeDB::bind_method(_MD("share", "share_params"),&Sdk::share);
 	ObjectTypeDB::bind_method(_MD("analytics", "params"),&Sdk::analytics);
 	ObjectTypeDB::bind_method(_MD("download", "url", "show_confirm", "force"),&Sdk::download);
}

Sdk *Sdk::get_singleton() {

	return instance;
}

Sdk::Sdk() {

	instance = this;
}

Sdk::~Sdk() {

	instance = NULL;
}

#endif // MODULE_SDK_ENABLED
