// Logs all output to a log file for the run
// Author: Max Schwarz <max.schwarz@uni-bonn.de>

#include "logger.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <stdexcept>

#include <sys/time.h>
#include <sys/stat.h>

#include <fmt/format.h>

namespace rosmon
{

Logger::Logger(const std::string& path, bool flush)
 : m_flush(flush)
{
	m_file = fopen(path.c_str(), "we");
	if(!m_file)
	{
		throw std::runtime_error(fmt::format(
			"Could not open log file: {}", strerror(errno)
		));
	}
}

Logger::~Logger()
{
	if(m_file)
		fclose(m_file);
}

void Logger::log(const LogEvent& event)
{
	struct timeval tv;
	memset(&tv, 0, sizeof(tv));
	gettimeofday(&tv, nullptr);

	struct tm btime;
	memset(&btime, 0, sizeof(tv));
	localtime_r(&tv.tv_sec, &btime);

	char timeString[100];
	strftime(timeString, sizeof(timeString), "%a %F %T", &btime);

	unsigned int len = event.message.length();
	while(len != 0 && (event.message[len-1] == '\n' || event.message[len-1] == '\r'))
		len--;

	// because of this problem:
	// https://serverfault.com/questions/221337/logrotate-successful-original-file-goes-back-to-original-size
	// we need this hack, but a better solution is at:
	// https://stackoverflow.com/questions/462122/detecting-that-log-file-has-been-deleted-or-truncated-on-posix-systems
	struct stat file_stat;
	int status = fstat(fileno(m_file), &file_stat);
	if (status == 0 && file_stat.st_size == 0) {
		// rewind the file pointer if the log file has been "copytruncated" by something like logrotate
		rewind(m_file);
	}

	fmt::print(m_file, "{}.{:03d}: {:>20}: ",
		timeString, tv.tv_usec / 1000,
		event.source.c_str()
	);
	fwrite(event.message.c_str(), 1, len, m_file);
	fputc('\n', m_file);

	if(m_flush)
		fflush(m_file);
}

}
