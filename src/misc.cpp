// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "misc.hpp"
#include <iostream> // std::cerr
#include <time.h> // time_t, time(), asctime_r()

namespace Asteria {

Logger::Logger(const char *file, unsigned long line, const char *func) noexcept
	: m_file(file), m_line(line), m_func(func)
{
	m_stream <<std::boolalpha;
}
Logger::~Logger(){
	//
}

void Logger::do_put(bool value){
	m_stream <<value;
}
void Logger::do_put(char value){
	m_stream <<value;
}
void Logger::do_put(signed char value){
	m_stream <<static_cast<int>(value);
}
void Logger::do_put(unsigned char value){
	m_stream <<static_cast<unsigned>(value);
}
void Logger::do_put(short value){
	m_stream <<static_cast<int>(value);
}
void Logger::do_put(unsigned short value){
	m_stream <<static_cast<unsigned>(value);
}
void Logger::do_put(int value){
	m_stream <<value;
}
void Logger::do_put(unsigned value){
	m_stream <<value;
}
void Logger::do_put(long value){
	m_stream <<value;
}
void Logger::do_put(unsigned long value){
	m_stream <<value;
}
void Logger::do_put(long long value){
	m_stream <<value;
}
void Logger::do_put(unsigned long long value){
	m_stream <<value;
}
void Logger::do_put(const char *value){
	m_stream <<value;
}
void Logger::do_put(const signed char *value){
	m_stream <<static_cast<const void *>(value);
}
void Logger::do_put(const unsigned char *value){
	m_stream <<static_cast<const void *>(value);
}
void Logger::do_put(const void *value){
	m_stream <<value;
}

void write_log_to_stderr(Logger &&logger) noexcept
try {
	// Prepend the timestamp and write the result to stderr.
	::time_t now;
	::time(&now);
	char time_str[26];
	::ctime_r(&now, time_str);
	time_str[24] = 0;
	auto &stream = logger.get_stream();
	stream <<'\n';
	stream.set_caret(0);
	stream <<"[" <<time_str <<"] " <<logger.get_file() <<":" <<logger.get_line() <<" ##: ";
	std::cerr <<stream.get_string() <<std::flush;
} catch(...){
	// Swallow any exceptions thrown.
}
void throw_runtime_error(Logger &&logger){
	// Prepend the function name and throw an `std::runtime_error`.
	auto &stream = logger.get_stream();
	stream.set_caret(0);
	stream <<logger.get_func() <<": ";
	throw std::runtime_error(stream.get_string());
}

void apply_indent(std::ostream &os, unsigned indent){
	if(indent == 0){
		return;
	}
	os <<std::setfill(' ') <<std::setw(static_cast<int>(indent)) <<"";
}
void quote_string(std::ostream &os, const std::string &str){
	os <<'\"';
	for(auto it = str.begin(); it != str.end(); ++it){
		const unsigned value = static_cast<unsigned char>(*it);
		switch(value){
		case '\"':
			os <<'\\' <<'\"';
			continue;
		case '\\':
			os <<'\\' <<'\\';
			continue;
		case '\a':
			os <<'\\' <<'a';
			continue;
		case '\b':
			os <<'\\' <<'b';
			continue;
		case '\f':
			os <<'\\' <<'f';
			continue;
		case '\n':
			os <<'\\' <<'n';
			continue;
		case '\r':
			os <<'\\' <<'r';
			continue;
		case '\t':
			os <<'\\' <<'t';
			continue;
		case '\v':
			os <<'\\' <<'v';
			continue;
		}
		if((value < 0x20) || (0x7E < value)){
			os <<'\\' <<'x' <<std::hex <<std::setfill('0') <<std::setw(2) <<value;
			continue;
		}
		os <<static_cast<char>(value);
	}
	os <<'\"';
}

}
