#!/usr/bin/env python3

# Python 3. At least 3.4.
# Requirements: ...
# Add argument to dump tz data to fs? And to load tz data dump from fs? It would be good for
# debugging and manual changing of archive if something goes wrong or information in db is wrong.

from urllib import request
import ssl
import tarfile
import io
import struct
import shutil
import os
from os import path
from datetime import datetime
from datetime import timezone as datetime_timezone


# SYSTEMTIME structure.
# https://docs.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-systemtime
class SystemTime:
	def __init__(self, year, month, day_of_week, day, hour, minute, second, milliseconds):
		assert isinstance(year, int) and 1601 <= year <= 30287
		assert isinstance(month, int) and 1 <= month <= 12
		assert isinstance(day_of_week, int) and 0 <= day_of_week <= 6
		assert isinstance(day, int) and 1 <= day <= 31
		assert isinstance(hour, int) and 0 <= hour <= 23
		assert isinstance(minute, int) and 0 <= minute <= 59
		assert isinstance(second, int) and 0 <= second <= 59
		assert isinstance(milliseconds, int) and 0 <= milliseconds <= 999

		self.year = year
		self.month = month
		self.day_of_week = day_of_week
		self.day = day
		self.hour = hour
		self.minute = minute
		self.second = second
		self.milliseconds = milliseconds

	def dump_bytes_le(self):
		return struct.pack(
			"<HHHHHHHH",
			self.year,
			self.month,
			self.day_of_week,
			self.day,
			self.hour,
			self.minute,
			self.second,
			self.milliseconds
		)


# REG_TZI_FORMAT structure.
# https://docs.microsoft.com/en-us/windows/win32/api/timezoneapi/ns-timezoneapi-time_zone_information
class RegistryTimezoneInformation:
	def __init__(self, bias, standard_bias, daylight_bias, standard_date, daylight_date):
		assert isinstance(bias, int) and -2 ** 31 <= bias <= 2 ** 31 - 1
		assert isinstance(standard_bias, int) and -2 ** 31 <= standard_bias <= 2 ** 31 - 1
		assert isinstance(daylight_bias, int) and -2 ** 31 <= daylight_bias <= 2 ** 31 - 1
		assert isinstance(standard_date, Date)
		assert isinstance(daylight_date, Date)

		self.bias = bias
		self.standard_bias = standard_bias
		self.daylight_bias = daylight_bias
		self.standard_date = standard_date
		self.daylight_date = daylight_date

	def dump_bytes_le(self):
		return (
			struct.pack("<iii", self.bias, self.standard_bias, self.daylight_bias) +
			self.standard_bias.dump_bytes_le() +
			self.daylight_date.dump_bytes_le()
		)


class Reader:
	EOF = None

	def __init__(self, stream_name, stream):
		assert isinstance(stream_name, str) and stream_name
		assert isinstance(stream, io.TextIOBase)

		self.stream_name = stream_name
		self.stream = stream
		self.line = 1
		self.position = 0
		self.character_to_read = None
		self.read()

	def read(self):
		character_read = self.character_to_read
		if character_read == '\n':
			self.line += 1
			self.position = 0
		else:
			self.position += 1
		try:
			self.character_to_read = stream.read(1)
		except EOFError:
			self.character_to_read = self.EOF
		return character_read

	def read_many(self, amount):
		return tuple(self.read() for i in range(amount))

	def read_one_of(self, characters):
		character_read = self.read()
		if character_read not in character:
			raise ParsingException(
				"expected one of %s, but found %s" % (
					repr(characters),
					repr(character_read)
				)
			)


class ParsingException(Exception):
	EXCEPTION_MESSAGE_FORMAT = "stream \"%s\", line %d, position %d: %s."

	def __init__(self, reader, message):
		assert isinstance(reader, Reader)
		super(self, EXCEPTION_MESSAGE_FORMAT % (reader.stream_name, reader.line, reader.position))


def character_range_to_set(first, last):
	return set(map(chr, range(ord(first), ord(last) + 1)))


class DayOfYear:
	MONTHS = ("Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec")
	DAYS_OF_WEEK = ("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")

	LAST_DAY_OF_MONTH = 0

	def __init__(self, ):
		pass

	# Month = "Jan" | "Feb" | "Mar" | "Apr" | "May" | "Jun" | "Jul" | "Aug" | "Sep" | "Oct" | "Nov" |
	# 	"Dec"
	@classmethod
	def read_month(cls, reader):
		assert map(len, cls.MONTHS) == [3] * len(cls.MONTHS)
		month = reader.read_many(3)
		if month not in cls.MONTHS:
			raise ParsingException("expected one of %s, but found %s." % (cls.MONTHS, month))
		return 1 + cls.MONTHS.index(month)

	# Day_of_week = "Mon" | "Tue" | "Wed" | "Thu" | "Fri" | "Sat" | "Sun"
	@classmethod
	def read_day_of_week(cls, reader):
		assert map(len, cls.DAYS_OF_WEEK) == [3] * len(cls.DAYS_OF_WEEK)
		day_of_week = reader.read_many(3)
		if day_of_week not in cls.DAYS_OF_WEEK:
			raise ParsingException("expected one of %s, but found %s." % (cls.DAYS_OF_WEEK, day_of_week))
		return 1 + cls.DAYS_OF_WEEK.index(day_of_week)

	# Day_of_month = (('1'..'2') ['1'..'9']) | ('3' ['0' | '1']) | ('4'..'9') # TODO: check that day
	# 	is present in month
	# Day_of_year = month ' ' (day_of_month | ("last" day_of_week) | (day_of_week [('>' | '<') '='
	# 	day_of_month]))
	@classmethod
	def read(cls, reader):
		month = cls.read_month(reader)
		reader.read_one_of((' ',))
		if reader.character_to_read == 'l':
			reader.read_one_of(('l',))
			reader.read_one_of(('a',))
			reader.read_one_of(('s',))
			reader.read_one_of(('t',))
			day_of_week = cls.read_day_of_week(reader)
			return cls(month, day_of_week, cls.LAST_DAY_OF_MONTH)
		else:
			day_of_week = cls.read_day_of_week(reader)
			if reader.character_to_read == '>':
				pass


class TimeInDay:
	pass


class TimeShift:
	pass


class Rule:
	NAME_CHARACTERS = (
		character_range_to_set('a', 'z') |
		character_range_to_set('A', 'Z') |
		character_range_to_set('0', '9')
	)
	ABBREVIATION_CHARACTERS = character_range_to_set('a', 'z') | character_range_to_set('A', 'Z')
	YEAR_FIRST_CHARACTER = character_range_to_set('1', '9')
	YEAR_CHARACTERS = character_range_to_set('0', '9')

	YEAR_MAX = 0
	YEAR_ONLY = -1

	def __init__(self, name, year_from, year_to, day_of_year, time_in_day, time_saved, abbreviation):
		assert isinstance(name, str) and name
		assert isinstance(year_from, int) and year_from >= 1
		assert isinstance(year_to, int) and (year_to >= year_from or year_to == cls.YEAR_MAX)
		assert isinstance(day_of_year, DayOfYear)
		assert isinstance(time_in_day, TimeInDay)
		assert isinstance(time_saved, int)
		assert isinstance(abbreviation, str) or abbreviation is None

		self.name = name
		self.year_from = year_from
		self.year_to = year_to
		self.day_of_year = day_of_year
		self.time_in_day = time_in_day
		self.time_saved = time_saved
		self.abbriviation = abbreviation

	def does_work_at(year):
		return self.year_from <= year <= self.year_to

	@staticmethod
	def read_definition_beginning(reader):
		reader.read_one_of(('R',))
		reader.read_one_of(('u',))
		reader.read_one_of(('l',))
		reader.read_one_of(('e',))

	# Year = ('1'..'9') {'0'..'9'}
	@classmethod
	def read_year(cls, reader):
		year = reader.read_one_of(cls.YEAR_FIRST_CHARACTER)
		while reader.character_to_read in cls.YEAR_CHARACTERS:
			year.append(reader.read())
		return int(''.join(year))

	# Year_to = year | "max" | "only"
	@classmethod
	def read_year_to(cls, reader):
		if reader.character_to_read == 'm':
			reader.read_one_of(('m',))
			reader.read_one_of(('a',))
			reader.read_one_of(('x',))
			return cls.YEAR_MAX
		elif reader.character_to_read == 'o':
			reader.read_one_of(('o',))
			reader.read_one_of(('n',))
			reader.read_one_of(('l',))
			reader.read_one_Of(('y',))
			return cls.YEAR_ONLY
		else:
			return cls.read_year(reader)

	# Name_character = 'a'..'z' | 'A'..'Z' | '0'..'9'
	# Name = name_character {name_character}
	# Abbreviation_character = 'a'..'z' | 'A'..'Z'
	# Abbreviation = '-' | (abbreviation_character {abbreviation_character})
	# Rule = "Rule" '\t' name '\t' year '\t' year_to '\t' '-' '\t' day_of_year '\t' time_in_day '\t'
	# 	abbreviation
	@classmethod
	def read(cls, reader):
		cls.read_definition_beginning()
		reader.read_one_of(('\t',))
		name = [reader.read_one_of(cls.NAME_CHARACTERS)]
		while reader.character_to_read in cls.NAME_CHARACTERS:
			rule_name.append(reader.read())
		reader.read_one_of(('\t',))
		year_from = cls.read_year(reader)
		reader.read_one_of(('\t',))
		year_to = cls.read_year_to(reader)
		if year_to == cls.YEAR_ONLY:
			year_to = year_from
		reader.read_one_of(('\t',))
		reader.read_one_of(('-',))
		reader.read_one_of(('\t',))
		day_of_year = DayOfYear.read(reader)
		reader.read_one_of(('\t',))
		time_in_day = TimeInDay.read(reader)
		reader.read_one_of(('\t',))
		abbreviation_first_character = reader.read_one_of(cls.ABBREVIATION_CHARACTERS | set('-'))
		abbreviation = None
		if abbreviation_first_character in cls.ABBREVIATION_CHARACTERS:
			abbreviation = [abbreviation_first_character]
			while reader.character_to_read in cls.ABBREVIATION_CHARACTERS:
				abbreviation.append(reader.read())
		return cls(''.join(name), year_from, year_to, day_of_year, time_in_day, abbreviation)


class Timezone:
	IANA_DB_REGION_FILENAMES = (
		"africa",
		"antarctica",
		"asia",
		"australasia",
		"europe",
		"northamerica",
		"southamerica",
		"etcetera"
	)
	CONVERTED_TIMEZONES_FILE_PATH = path.join(
		path.dirname(__file__),
		"boot",
		"bootdata",
		"timezones.inf"
	)

	@classmethod
	def download_timezones_archive(cls):
		print("Timezone.download_timezones_archive: started downloading timezones archive.")
		response = request.urlopen(
			"https://www.iana.org/time-zones/repository/tzdata-latest.tar.gz",
			context=ssl.create_default_context()
		)
		content = response.read()
		archive = tarfile.open(fileobj=io.BytesIO(content), mode="r:gz")
		print("Timezone.download_timezones_archive: done downloading timezones archive.")
		return archive

	@classmethod
	def extract_timezones(cls, timezones_file, timezones_file_name):
		pass

	@classmethod
	def extract_timezones_from_archive(cls, archive):
		print("Timezone.extract_timezones_from_archive: started extracting timezones from archive.")
		timezones = []
		for archived_file in archive:
			if archived_file.name in cls.IANA_DB_REGION_FILENAMES:
				timezones += cls.extract_timezones(archive.extractfile(archived_file), archived_file.name)
		print("Timezone.extract_timezones_from_archive: done extracting timezones from archive.")
		return timezones

	@classmethod
	def convert_timezones(timezones):
		pass

	@classmethod
	def update_timezones(cls):
		print("Timezone.update_timezones: started updating timezones.")
		archive = cls.download_timezones_archive()
		timezones = cls.extract_timezones_from_archive(archive)
		inf_output = cls.convert_timezones_to_inf(timezones)
		print("Timezone.update_timezones: started writing inf output.")
		with open(cls.CONVERTED_TIMEZONES_FILE_PATH, 'w') as stream:
			stream.write(inf_output)
		print("Timezone.update_timezones: done writing inf output.")
		print("Timezone.update_timezones: done updating timezones.")


if __name__ == "__main__":
	Timezone.update_timezones()
