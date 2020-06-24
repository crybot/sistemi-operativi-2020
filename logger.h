#ifndef LOGGER_H
#define LOGGER_H

void log_setfile(const char *filename);
void log_write(const char *format, ...);
void log_close(void);

#endif
