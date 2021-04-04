#!/usr/bin/env python3

# Python 3. At least 3.4.
# Requirements: ...
# Load files from existing_directory if present. Maybe add archive name to the end of the
# directory?
# TODO: refactor and simplify if possible. Self-review the code.
# TODO: perform substitutions? It may shrink the code.
# Add argument to dump tz data to fs? And to load tz data dump from fs? It would be good for
# debugging and manual changing of archive if something goes wrong or information in db is wrong.
# Complete syntax at the top of the file? Some comments and decisions that script makes when
# converting timzones to inf format? Standard copyright header (with author, project and etc)?
# Maybe, the code has too many classes.

# Comment = '#' {^'\n'} '\n'
# Separator_character = '\t' | ' '
# Separator = separator_character {separator_character}
# Number = '0' | ('1'..'9' {'0'..'9'})

# Year = '1'..'9' {'0'..'9'}
# Month = "Jan" | "Feb" | "Mar" | "Apr" | "May" | "Jun" | "Jul" | "Aug" | "Sep" | "Oct" | "Nov" |
# 	"Dec"
# Day_of_week = "Mon" | "Tue" | "Wed" | "Thu" | "Fri" | "Sat" | "Sun"
# I noticed a zone which has leading zeros in the until column before the day number.
# Day = {'0'} ((('1'..'2') ['1'..'9']) | ('3' ['0' | '1']) | ('4'..'9'))
## Hours = '0' | ('1' ['0'..'9']) | ('2' ['0'..'3']) | ('3'..'9') # Not used at the moment.
# Minutes_02d = '0'..'5' '0'..'9'
# Seconds_02d = '0'..'5' '0'..'9'

# Number of hours can be greater than 23 in the "at" column of a rule.
# Offset = number ':' minutes_02d [':' seconds_02d]
# Offset_signed = ['-'] offset
# Time_in_day = offset ['g' | 's' | 'u' | 'z' | 'w']
# Timestamp = year [separator [month [separator [day [separator [time_in_day]]]]]]

# Day_of_month = day | ("last" day_of_week) | (day_of_week ('<' | '>') '=' day)
# Day_of_year = month separator day_of_month

# Hours_without_zero = ('1' ['0'..'9']) | ('2' ['0'..'3']) | '3'..'9'
# Offset_or_zero = ('0' [':' minutes_02d]) | (hours_without_zero ':' minutes_02d)
# Rule_name_character = 'a'..'z' | 'A'..'Z' | '0'..'9'
# Rule_name = rule_name_character {rule_name_character}
# Year_to = year | "max" | "only"
# Abbreviation_character = 'a'..'z' | 'A'..'Z'
# Abbreviation = '-' | (abbreviation_character {abbreviation_character})
# Rule = "Rule" separator rule_name separator year separator year_to separator '-' separator
# 	day_of_year separator time_in_day separator offset_or_zero separator abbreviation {' '} (comment
# 	| '\n')

# Rules = '-' | offset | rule_name
# Format_character = 'A'..'Z' | 'a'..'z' | '0'..'9' | '+' | '-'
# Format_element = format_character | "%s"
# Format = format_element {format_element}
# Until = timestamp # First moment when the rule is to be used.
# Record = offset_signed separator rules separator format {' '} ((['\t'] timestamp {' '} (comment |
# 	'\n')) | '\n')

# Zone_name_part = {'A'..'Z' | 'a'..'z' | '_'}
# Zone_name = ('A'..'Z') zone_name_part '/' ('A'..'Z') zone_name_part
# Zone = "Zone" separator zone_name separator record {'\t' {'\t' | ' '} {comment} record}

# Link = "Link" separator zone_name separator zone_name {' ' | '\t'} (comment | '\n')

# Line = rule | zone | link | comment | '\n'
# Region = line {line}

# TODO: check the rules by combining them into a single big one.
# TODO: check that the definitions above functions are the same as the definitions
# 	above this notice.

# TODO: name variables to self-document code as well. No need to follow the definition.
# TODO: figure out whether "repr" should be used or something else. A string representation is
# 	needed: escaping unprintatble characters and quotes for strings, numbers are left unchanged,
# 	iterables are enumerated. If order of elements doesn't matter, iterable is displayed as set.
# TODO: check manual-conformity of the syntax.


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


class Reader:
	EOF = None

	def __init__(self, stream_name, stream):
		assert isinstance(stream_name, str) and stream_name
		assert isinstance(stream, io.TextIOBase)

		self.stream_name = stream_name
		self.stream = stream
		self.line = 1
		self.position = 0
		self.next_character = None
		self.read()

	def read(self):
		character_read = self.next_character
		if character_read == '\n':
			self.line += 1
			self.position = 0
		else:
			self.position += 1
		self.next_character = self.stream.read(1) or self.EOF
		return character_read

	def check_eof(self):
		if self.next_character != self.EOF:
			raise ParsingException(self, "expected end of file, but found %s" % repr(self.next_character))

	def read_many(self, amount):
		return tuple(self.read() for i in range(amount))

	def read_one_of(self, characters):
		character_read = self.read()
		if character_read not in characters:
			raise ParsingException(
				self,
				"expected one of %s, but found %s" % (
					repr(characters),
					repr(character_read)
				)
			)
		return character_read


class ParsingException(Exception):
	EXCEPTION_MESSAGE_FORMAT = "stream \"%s\", line %d, position %d: %s."

	def __init__(self, reader, message):
		assert isinstance(reader, Reader)
		super().__init__(
			self.EXCEPTION_MESSAGE_FORMAT % (reader.stream_name, reader.line, reader.position, message)
		)


class RuleReferencingException(Exception):
	pass


class ZoneReferencingException(Exception):
	pass


def character_range_to_set(first, last):
	return set(map(chr, range(ord(first), ord(last) + 1)))


class Common:
	# Comment = '#' {^'\n'} '\n'
	@staticmethod
	def read_comment(reader):
		reader.read_one_of(('#',))
		while reader.next_character not in ('\n', reader.EOF):
			reader.read()
		reader.read_one_of(('\n',))

	# NOTE: sometimes there are spaces when a delimiter should appear. Even many of them. We
	# 	introduce the separator entity to deal with this.
	# Separator_character = '\t' | ' '
	# Separator = separator_character {separator_character}
	SEPARATOR_BEGINNINGS = set(('\t', ' '))
	SEPARATOR_CHARACTERS = set(('\t', ' '))
	@classmethod
	def read_separator(cls, reader):
		reader.read_one_of(cls.SEPARATOR_CHARACTERS)
		while reader.next_character in cls.SEPARATOR_CHARACTERS:
			reader.read()

	POSITIVE_NUMBER_BEGINNINGS = character_range_to_set('1', '9')
	POSITIVE_NUMBER_CHARACTERS = character_range_to_set('0', '9')
	# Positive_number = '1'..'9' {'0'..'9'}
	@classmethod
	def read_positive_number(cls, reader):
		number = [reader.read_one_of(cls.POSITIVE_NUMBER_BEGINNINGS)]
		while reader.next_character in cls.POSITIVE_NUMBER_CHARACTERS:
			number.append(reader.read())
		return int(''.join(number))

	NUMBER_BEGINNINGS = set(('0',)) | POSITIVE_NUMBER_BEGINNINGS
	# Number = ('0' {'0'} [positive_number]) | positive_number
	@classmethod
	def read_number(cls, reader):
		if reader.next_character == '0':
			reader.read()
			while reader.next_character == '0':
				reader.read()
			if reader.next_character in cls.POSITIVE_NUMBER_BEGINNINGS:
				return cls.read_positive_number(reader)
			else:
				return 0
		else:
			return cls.read_positive_number(reader)

	YEAR_BEGINNINGS = character_range_to_set('1', '9')
	YEAR_CHARACTERS = character_range_to_set('0', '9')
	# Year = ('1'..'9') {'0'..'9'}
	@classmethod
	def read_year(cls, reader):
		year = [reader.read_one_of(cls.YEAR_BEGINNINGS)]
		while reader.next_character in cls.YEAR_CHARACTERS:
			year.append(reader.read())
		return int(''.join(year))

	# TODO: rethink this. Seemingly it is not EBNF.
	MONTH_BEGINNINGS = set(('J', 'F', 'M', 'A', 'M', 'J', 'J', 'A', 'S', 'O', 'N', 'D'))
	MONTHS = ("Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec")
	# Month = "Jan" | "Feb" | "Mar" | "Apr" | "May" | "Jun" | "Jul" | "Aug" | "Sep" | "Oct" | "Nov" |
	# 	"Dec"
	@classmethod
	def read_month(cls, reader):
		assert list(map(len, cls.MONTHS)) == [3] * len(cls.MONTHS)
		month = reader.read_many(3)
		if month is reader.EOF or ''.join(month) not in cls.MONTHS:
			raise ParsingException(
				reader,
				"expected one of %s, but found %s" % (repr(cls.MONTHS), repr(month))
			)
		return 1 + cls.MONTHS.index(''.join(month))

	DAY_BEGINNINGS = character_range_to_set('1', '9') | set(('0',)) # TODO: consider not touching day?
	# 	It is common, so may be reused.
	# Day = {'0'} ((('1'..'2') ['0'..'9']) | ('3' ['0' | '1']) | ('4'..'9')) # TODO: check that day is
	# 	present in month
	@staticmethod
	def read_day(reader):
		while reader.next_character == '0':
			reader.read()
		if reader.next_character in ('1', '2'):
			number = [reader.read()]
			if reader.next_character in character_range_to_set('0', '9'):
				number.append(reader.read())
			return int(''.join(number))
		elif reader.next_character == '3':
			day_of_month = [reader.read()]
			if reader.next_character in ('0', '1'):
				day_of_month.append(reader.read())
			return int(''.join(day_of_month))
		else:
			return int(reader.read_one_of(character_range_to_set('4', '9')))

	HOURS_POSITIVE_BEGINNINGS = character_range_to_set('1', '9')
	@staticmethod
	def read_hours_positive(reader):
		if reader.next_character == '1':
			hours = [reader.read()]
			if reader.next_character in character_range_to_set('0', '9'):
				hours.append(reader.read())
			return int(''.join(hours))
		elif reader.next_character == '2':
			hours = [reader.read()]
			if reader.next_character in character_range_to_set('0', '3'):
				hours.append(reader.read())
			return int(''.join(hours))
		else:
			return int(reader.read_one_of(character_range_to_set('3', '9')))

	HOURS_BEGINNINGS = set(('0',)) | HOURS_POSITIVE_BEGINNINGS
	# Hours_positive = ('1' ['0'..'9']) | ('2' ['0'..'3']) | ('3'..'9')
	# Hours = ('0' {'0'} [hours_positive]) | hours_positive
	@classmethod
	def read_hours(cls, reader):
		if reader.next_character == '0':
			reader.read()
			while reader.next_character == '0':
				reader.read()
			if reader.next_character in cls.HOURS_POSITIVE_BEGINNINGS:
				return cls.read_hours_positive(reader)
			else:
				return 0
		else:
			return cls.read_hours_positive(reader)

	# Minutes_02d = ('0'..'5') ('0'..'9')
	@staticmethod
	def read_minutes_02d(reader):
		return int(
			reader.read_one_of(character_range_to_set('0', '5')) +
			reader.read_one_of(character_range_to_set('0', '9'))
		)

	# Seconds_02d = ('0'..'5') ('0'..'9')
	@staticmethod
	def read_seconds_02d(reader):
		return int(
			reader.read_one_of(character_range_to_set('0', '5')) +
			reader.read_one_of(character_range_to_set('0', '9'))
		)

	DAY_OF_WEEK_BEGINNINGS = set(('M', 'T', 'W', 'T', 'F', 'S', 'S'))
	DAYS_OF_WEEK = ("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
	# Day_of_week = "Mon" | "Tue" | "Wed" | "Thu" | "Fri" | "Sat" | "Sun"
	@classmethod
	def read_day_of_week(cls, reader):
		assert list(map(len, cls.DAYS_OF_WEEK)) == [3] * len(cls.DAYS_OF_WEEK)
		day_of_week = ''.join(reader.read_many(3))
		if day_of_week not in cls.DAYS_OF_WEEK:
			raise ParsingException(
				reader,
				"expected one of %s, but found %s" % (cls.DAYS_OF_WEEK, day_of_week)
			)
		return 1 + cls.DAYS_OF_WEEK.index(day_of_week)


class Offset:
	def __init__(self, hours, minutes, seconds):
		assert isinstance(hours, int) and hours >= 0
		assert isinstance(minutes, int) and 0 <= minutes <= 59
		assert isinstance(seconds, int) and 0 <= seconds <= 59

		self.hours = hours
		self.minutes = minutes
		self.seconds = seconds
		self.is_negative = False

	BEGINNINGS = Common.NUMBER_BEGINNINGS
	# TODO: looks like "hours" is still not used.
	# Offset = number [':' minutes_02d [':' seconds_02d]]
	@classmethod
	def from_stream(cls, reader):
		hours = Common.read_number(reader)
		minutes = 0
		seconds = 0
		if reader.next_character == ':':
			reader.read()
			minutes = Common.read_minutes_02d(reader)
			if reader.next_character == ':':
				reader.read()
				seconds = Common.read_seconds_02d(reader)
		return cls(hours, minutes, seconds)

	BEGINNINGS_IF_SIGNED = BEGINNINGS | set(('-',))
	BEGINNINGS_IF_SIGNED_OR_NONE = BEGINNINGS_IF_SIGNED
	# Offset_signed = ['-'] offset
	# Offset_signed_or_none = ['-'] [offset]
	@classmethod
	def signed_from_stream(cls, reader, may_be_absent=False):
		if reader.next_character == '-':
			reader.read()
			if not may_be_absent or reader.next_character in cls.BEGINNINGS:
				instance = cls.from_stream(reader)
				instance.is_negative = True
				return instance
			else:
				return None
		else:
			return cls.from_stream(reader)


class DayOfMonth:
	NUMBER_EQUALS = 0
	DAY_OF_WEEK_WITH_NUMBER_LEQ = 1
	DAY_OF_WEEK_WITH_NUMBER_GEQ = 2
	LAST_DAY_OF_WEEK_IN_MONTH = 3

	def __init__(self, condition, day, day_of_week=None):
		assert condition in (
			self.NUMBER_EQUALS,
			self.DAY_OF_WEEK_WITH_NUMBER_LEQ,
			self.DAY_OF_WEEK_WITH_NUMBER_GEQ,
			self.LAST_DAY_OF_WEEK_IN_MONTH
		)

		if condition == self.NUMBER_EQUALS:
			assert isinstance(day, int) and 1 <= day <= 31
			assert day_of_week is None

			self.condition = condition
			self.day = day
		elif condition in (self.DAY_OF_WEEK_WITH_NUMBER_LEQ, self.DAY_OF_WEEK_WITH_NUMBER_GEQ):
			assert isinstance(day, int) and 1 <= day <= 31
			assert isinstance(day_of_week, int) and 1 <= day_of_week <= 7

			self.day = day
			self.day_of_week = day_of_week
		elif condition == self.LAST_DAY_OF_WEEK_IN_MONTH:
			assert day is None
			assert isinstance(day_of_week, int) and 1 <= day_of_week <= 7

			self.day_of_week = day_of_week

	BEGINNINGS = Common.DAY_BEGINNINGS | set(('l',)) | Common.DAY_OF_WEEK_BEGINNINGS
	# Day_of_month = day | ("last" day_of_week) | (day_of_week ('<' | '>') '=' day)
	@classmethod
	def from_stream(cls, reader):
		if reader.next_character in Common.DAY_BEGINNINGS:
			day = Common.read_day(reader)
			return cls(cls.NUMBER_EQUALS, day)
		elif reader.next_character == 'l':
			reader.read_one_of(('l',))
			reader.read_one_of(('a',))
			reader.read_one_of(('s',))
			reader.read_one_of(('t',))
			day_of_week = Common.read_day_of_week(reader)
			return cls(cls.LAST_DAY_OF_WEEK_IN_MONTH, None, day_of_week)
		else:
			day_of_week = Common.read_day_of_week(reader)
			comparison_character = reader.read_one_of(('<', '>'))
			condition = None
			if comparison_character == '<':
				condition = cls.DAY_OF_WEEK_WITH_NUMBER_LEQ
			elif comparison_character == '>':
				condition = cls.DAY_OF_WEEK_WITH_NUMBER_GEQ
			reader.read_one_of(('=',))
			day = Common.read_day(reader)
			return cls(condition, day, day_of_week)


class DayOfYear:
	def __init__(self, month, day_of_month):
		assert isinstance(month, int) and 1 <= month <= 12
		assert isinstance(day_of_month, DayOfMonth)

		self.month = month
		self.day_of_month = day_of_month

	# Day_of_year = month separator day_of_month
	@classmethod
	def from_stream(cls, reader):
		month = Common.read_month(reader)
		Common.read_separator(reader)
		day_of_month = DayOfMonth.from_stream(reader)
		return cls(month, day_of_month)


class TimeInDay:
	# Or "is_global"? And then change the tuple of variants in "from_stream" function.
	def __init__(self, offset, is_local_wall_time, is_local_standard_time):
		assert isinstance(offset, Offset) and not offset.is_negative
		assert isinstance(is_local_wall_time, bool)
		assert isinstance(is_local_standard_time, bool)

		self.offset = offset
		self.is_local_wall_time = is_local_wall_time
		self.is_local_standard_time = is_local_standard_time

	BEGINNINGS = Offset.BEGINNINGS
	TIME_FORMAT_CHARACTERS = set(('g', 's', 'u', 'w', 'z'))
	# Time_in_day = offset ['g' | 's' | 'u' | 'z' | 'w']
	@classmethod
	def from_stream(cls, reader):
		offset = Offset.from_stream(reader)
		time_format_character = None
		if reader.next_character in cls.TIME_FORMAT_CHARACTERS:
			time_format_character = reader.read_one_of(cls.TIME_FORMAT_CHARACTERS)
		return cls(offset, time_format_character in (None, 'w'), time_format_character == 's')


class Timestamp:
	def __init__(self, year, month, day_of_month, time_in_day):
		assert isinstance(year, int) and year >= 1
		assert isinstance(month, int) and 1 <= month <= 12
		assert isinstance(day_of_month, DayOfMonth)
		assert isinstance(time_in_day, TimeInDay)

		self.year = year
		self.month = month
		self.day_of_month = day_of_month
		self.time_in_day = time_in_day

	BEGINNINGS = Common.YEAR_BEGINNINGS
	# Timestamp = year [separator [month [separator [day_of_month [separator [time_in_day]]]]]]
	@classmethod
	def from_stream(cls, reader):
		year = Common.read_year(reader)
		month = 1
		day_of_month = DayOfMonth(DayOfMonth.NUMBER_EQUALS, 1)
		time_in_day = TimeInDay(Offset(0, 0, 0), is_local_wall_time=True, is_local_standard_time=False)
		if reader.next_character in Common.SEPARATOR_CHARACTERS:
			Common.read_separator(reader)
			if reader.next_character in Common.MONTH_BEGINNINGS:
				month = Common.read_month(reader)
				if reader.next_character in Common.SEPARATOR_CHARACTERS:
					Common.read_separator(reader)
					if reader.next_character in DayOfMonth.BEGINNINGS:
						day_of_month = DayOfMonth.from_stream(reader)
						if reader.next_character in Common.SEPARATOR_CHARACTERS:
							Common.read_separator(reader)
							if reader.next_character in TimeInDay.BEGINNINGS:
								time_in_day = TimeInDay.from_stream(reader)
		return cls(year, month, day_of_month, time_in_day)


class Rule:
	YEAR_MAX = 0
	YEAR_ONLY = -1

	def __init__(self, name, year_from, year_to, day_of_year, time_in_day, time_saved, abbreviation):
		assert isinstance(name, str) and name
		assert isinstance(year_from, int) and year_from >= 1
		assert isinstance(year_to, int) and (year_to >= year_from or year_to == self.YEAR_MAX)
		assert isinstance(day_of_year, DayOfYear)
		assert isinstance(time_in_day, TimeInDay)
		assert isinstance(time_saved, Offset)
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

	NAME_BEGINNINGS = character_range_to_set('a', 'z') | character_range_to_set('A', 'Z')
	NAME_CHARACTERS = (
		character_range_to_set('a', 'z') |
		character_range_to_set('A', 'Z') |
		character_range_to_set('0', '9') |
		set(('-', '_'))
	)
	# Rule_name = ('a'..'z' | 'A'..'Z' | '_') {'a'..'z' | 'A'..'Z' | '0'..'9' | '-' | '_'}
	@classmethod
	def read_name(cls, reader):
		name = [reader.read_one_of(cls.NAME_BEGINNINGS)]
		while reader.next_character in cls.NAME_CHARACTERS:
			name.append(reader.read())
		return ''.join(name)

	# Year_to = year | "max" | "only"
	@classmethod
	def read_year_to(cls, reader):
		if reader.next_character == 'm':
			reader.read()
			reader.read_one_of(('a',))
			reader.read_one_of(('x',))
			return cls.YEAR_MAX
		elif reader.next_character == 'o':
			reader.read()
			reader.read_one_of(('n',))
			reader.read_one_of(('l',))
			reader.read_one_of(('y',))
			return cls.YEAR_ONLY
		else:
			return Common.read_year(reader)

	# Hours_without_zero = ('1' ['0'..'9']) | ('2' ['0'..'3']) | '3'..'9'
	@staticmethod
	def read_hours_without_zero(reader):
		if reader.next_character == '1':
			hours = [reader.read()]
			if reader.next_character in character_range_to_set('0', '9'):
				hours.append(reader.read())
			return int(''.join(hours))
		elif reader.next_character == '2':
			hours = [reader.read()]
			if reader.next_character in character_range_to_set('0', '3'):
				hours.append(reader.read())
			return int(''.join(hours))
		else:
			return int(reader.read_one_of(character_range_to_set('3', '9')))

	ABBREVIATION_CHARACTERS = (
		set(('+', '-')) |
		character_range_to_set('0', '9') |
		character_range_to_set('a', 'z') |
		character_range_to_set('A', 'Z')
	)
	# Abbreviation_character = '+' | '-' | '0'..'9' | 'a'..'z' | 'A'..'Z'
	# Abbreviation = abbreviation_character {abbreviation_character}
	# If the abbreviation is "-", then it is absent (that is not checked by the syntax itself).
	@classmethod
	def read_abbreviation(cls, reader):
		abbreviation = [reader.read_one_of(cls.ABBREVIATION_CHARACTERS)]
		while reader.next_character in cls.ABBREVIATION_CHARACTERS:
			abbreviation.append(reader.read())
		if abbreviation == ['-']:
			return None
		else:
			return ''.join(abbreviation)

	# Rule = "Rule" separator name separator year separator year_to separator '-' separator
	# 	day_of_year separator time_in_day separator offset_signed separator abbreviation {' ' | '\t'}
	# 	(comment | '\n')
	@classmethod
	def from_stream(cls, reader):
		reader.read_one_of(('R',))
		reader.read_one_of(('u',))
		reader.read_one_of(('l',))
		reader.read_one_of(('e',))
		Common.read_separator(reader)
		name = cls.read_name(reader)
		Common.read_separator(reader)
		year_from = Common.read_year(reader)
		Common.read_separator(reader)
		year_to = cls.read_year_to(reader)
		if year_to == cls.YEAR_ONLY:
			year_to = year_from
		Common.read_separator(reader)
		reader.read_one_of(('-',))
		Common.read_separator(reader)
		day_of_year = DayOfYear.from_stream(reader)
		Common.read_separator(reader)
		time_in_day = TimeInDay.from_stream(reader)
		Common.read_separator(reader)
		time_saved = Offset.signed_from_stream(reader)
		Common.read_separator(reader)
		abbreviation = cls.read_abbreviation(reader)
		while reader.next_character in (' ', '\t'):
			reader.read()
		if reader.next_character == '#':
			Common.read_comment(reader)
		else:
			reader.read_one_of(('\n',))
		return cls(name, year_from, year_to, day_of_year, time_in_day, time_saved, abbreviation)


class Record:
	def __init__(self, offset, rules, format, until):
		assert isinstance(offset, Offset)
		assert rules is None or (isinstance(rules, str) and rules) or isinstance(rules, Offset)
		assert (isinstance(format, str) and format) or (isinstance(format, tuple) and len(format) == 2)
		assert until is None or isinstance(until, Timestamp)

		self.offset = offset
		self.rules = rules
		self.format = format
		self.until = until

	# Rules = offset_signed_or_none | rule_name
	@classmethod
	def read_rules(cls, reader):
		if reader.next_character in Offset.BEGINNINGS_IF_SIGNED_OR_NONE:
			return Offset.signed_from_stream(reader, may_be_absent=True)
		else:
			rule_name = Rule.read_name(reader)
			return rule_name

	FORMAT_CHARACTERS = (
		character_range_to_set('A', 'Z') |
		character_range_to_set('a', 'z') |
		character_range_to_set('0', '9') |
		set(('+', '-'))
	)
	# Format_character = 'A'..'Z' | 'a'..'z' | '0'..'9' | '+' | '-'
	# Format_element = format_character | "%s"
	@classmethod
	def read_format_element(cls, reader):
		if reader.next_character in cls.FORMAT_CHARACTERS:
			return reader.read()
		else:
			return reader.read_one_of(('%',)) + reader.read_one_of(('s',))

	# TODO: do checks, that are not performed by the definition itself.
	# Format = format_element {format_element} ['/' format_element {format_element}]
	@classmethod
	def read_format(cls, reader, may_be_a_pattern=False, rule_name_was_provided=False):
		format = [cls.read_format_element(reader)]
		while reader.next_character in cls.FORMAT_CHARACTERS or reader.next_character == '%':
			format.append(cls.read_format_element(reader))
		if reader.next_character == '/':
			reader.read()
			format_ds_time = [cls.read_format_element(reader)]
			while reader.next_character in cls.FORMAT_CHARACTERS or reader.next_character == '%':
				format_ds_time.append(cls.read_format_element(reader))
			format_std_time = ''.join(format)
			format_ds_time = ''.join(format_ds_time)
			if "%s" in format_std_time or "%s" in format_ds_time:
				raise ParsingException(
					reader,
					(
						"format should not contain \"%s\" if both standard time and daylight saving time " +
						("abbreviations are given (got %s)" % repr(format_std_time + '/' + format_ds_time))
					)
				)
			return (format_std_time, format_ds_time)
		return ''.join(format)

	BEGINNINGS = Offset.BEGINNINGS_IF_SIGNED
	# Record = offset_signed separator rules separator format ('\n' | comment | (separator ('\n' |
	# 	comment | (timestamp [separator] (comment | '\n')))))
	@classmethod
	def from_stream(cls, reader):
		offset = Offset.signed_from_stream(reader)
		Common.read_separator(reader)
		rules = cls.read_rules(reader)
		Common.read_separator(reader)
		format = cls.read_format(reader)
		if (rules is None or isinstance(rules, Offset)) and isinstance(format, str) and "%s" in format:
			raise ParsingException(
				reader,
				(
					"format should not contain \"%s\" if rule was not given or it is an offset " +
					("(got %s)" % repr(format))
				)
			)
		until = None
		if reader.next_character == '\n':
			reader.read()
		elif reader.next_character == '#':
			Common.read_comment(reader)
		elif reader.next_character in (' ', '\t'):
			Common.read_separator(reader)
			if reader.next_character == '\n':
				reader.read()
			elif reader.next_character == '#':
				Common.read_comment(reader)
			else:
				until = Timestamp.from_stream(reader)
				if reader.next_character in (' ', '\t'):
					Common.read_separator(reader)
				if reader.next_character == '#':
					Common.read_comment(reader)
				else:
					reader.read_one_of(('\n',))
		return cls(offset, rules, format, until)


class Zone:
	def __init__(self, name, record):
		assert isinstance(name, str) and name
		assert isinstance(record, Record)

		self.name = name
		self.records = [record]

	def add_record(self, record):
		assert isinstance(record, Record)

		self.records.append(record)

	NAME_BEGINNINGS = character_range_to_set('A', 'Z')
	NAME_CHARACTERS = (
		character_range_to_set('A', 'Z') |
		character_range_to_set('a', 'z') |
		character_range_to_set('0', '9') |
		set(('_', '-', '+', '/'))
	)
	# Zone_name = ('A'..'Z') {'A'..'Z' | 'a'..'z' | '0'..'9' | '_' | '-' | '+' | '/'}
	@classmethod
	def read_name(cls, reader):
		name = [reader.read_one_of(cls.NAME_BEGINNINGS)]
		while reader.next_character in cls.NAME_CHARACTERS:
			name.append(reader.read())
		return ''.join(name)

	# Zone = "Zone" separator zone_name separator record
	@classmethod
	def from_stream(cls, reader):
		reader.read_one_of(('Z',))
		reader.read_one_of(('o',))
		reader.read_one_of(('n',))
		reader.read_one_of(('e',))
		Common.read_separator(reader)
		name = cls.read_name(reader)
		Common.read_separator(reader)
		record = Record.from_stream(reader)
		return cls(name, record)


class Region:
	# TODO: we have similar situation with "record". Maybe use the construction of comment and newline
	# 	from there?
	# Link = "Link" separator zone_name separator zone_name {' ' | '\t'} (comment | '\n')
	@staticmethod
	def read_link(reader):
		reader.read_one_of(('L',))
		reader.read_one_of(('i',))
		reader.read_one_of(('n',))
		reader.read_one_of(('k',))
		Common.read_separator(reader)
		src = Zone.read_name(reader)
		Common.read_separator(reader)
		dst = Zone.read_name(reader)
		while reader.next_character in (' ', '\t'):
			reader.read()
		if reader.next_character == '#':
			Common.read_comment(reader)
		else:
			reader.read_one_of(('\n',))
		return (src, dst)

	# Line = {' ' | '\t'} (comment | rule | zone | record | link)
	@classmethod
	def read_line(cls, reader, rules_by_name, zone_by_name, links, current_zone):
		while reader.next_character in (' ', '\t'):
			reader.read()
		if reader.next_character == '#':
			Common.read_comment(reader)
		elif reader.next_character == 'R':
			rule = Rule.from_stream(reader)
			if rule.name not in rules_by_name:
				rules_by_name[rule.name] = []
			rules_by_name[rule.name].append(rule)
		elif reader.next_character == 'Z':
			zone = Zone.from_stream(reader)
			if zone.name in zone_by_name:
				raise ParsingException(reader, "redefinition of zone %s" % repr(zone.name))
			zone_by_name[zone.name] = zone
			current_zone = zone
		elif reader.next_character in Record.BEGINNINGS:
			record = Record.from_stream(reader)
			if current_zone is None:
				raise ParsingException(
					reader,
					"a standalone record before all of the zones (it must follow the zone it is related to)"
				)
			current_zone.add_record(record)
		elif reader.next_character == 'L':
			links.append(cls.read_link(reader))
		else:
			reader.read_one_of(('\n',))
		return current_zone

	@staticmethod
	def resolve_rule_references(rules_by_name, zone_by_name, stream_name):
		for zone in zone_by_name.values():
			for record_index, record in enumerate(zone.records):
				if isinstance(record.rules, str):
					ruleset_name = record.rules
					if ruleset_name not in rules_by_name:
						raise RuleReferencingException(
							(
								"stream \"%s\", zone \"%s\", record #%d: the referenced rule (\"%s\") was not " +
								"defined."
							) % (stream_name, zone.name, record_index, ruleset_name)
						)
					record.rules = rules_by_name[ruleset_name]

	@staticmethod
	def resolve_zone_references(links, zone_by_name, stream_name):
		for link_index, link in enumerate(links):
			src, dst = link
			if src not in zone_by_name:
				raise ZoneReferencingException(
					"stream \"%s\", link #%d from \"%s\" to \"%s\": the source zone was not defined."
					% (stream_name, link_index, src, dst)
				)
			if dst in zone_by_name:
				raise ZoneReferencingException(
					"stream \"%s\", link #%d from \"%s\" to \"%s\": redefinition of the destination zone."
					% (stream_name, link_index, src, dst)
				)
			zone_by_name[dst] = zone_by_name[src]


	# Region = Line {Line}
	@classmethod
	def from_stream(cls, reader):
		rules_by_name = {}
		zone_by_name = {}
		links = []
		current_zone = None
		current_zone = cls.read_line(reader, rules_by_name, zone_by_name, links, current_zone)
		while reader.next_character in ('R', 'Z', 'L', '#', '\t', ' ', '\n'):
			current_zone = cls.read_line(reader, rules_by_name, zone_by_name, links, current_zone)
		reader.check_eof()
		cls.resolve_rule_references(rules_by_name, zone_by_name, reader.stream_name)
		cls.resolve_zone_references(links, zone_by_name, reader.stream_name)
		return zone_by_name


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


REGION_FILES = (
	"africa",
	"antarctica",
	"asia",
	"australasia",
	"europe",
	"northamerica",
	"southamerica",
	"etcetera"
)
EXTRACTED_FILES_DIR = path.join(path.dirname(__file__), "timezones_tmp")
CONVERTED_TIMEZONES_FILE = path.join(path.dirname(__file__), "boot", "bootdata", "timezones.inf")


def download_archive(tzdata_version):
	print("download_archive: started.")
	url = None
	if tzdata_version is None:
		url = "https://www.iana.org/time-zones/repository/tzdata-latest.tar.gz"
	else:
		url = "https://data.iana.org/time-zones/releases/tzdata%s.tar.gz" % tzdata_version
	response = request.urlopen(url, context=ssl.create_default_context())
	content = response.read()
	archive = tarfile.open(fileobj=io.BytesIO(content), mode="r:gz")
	print("download_archive: done.")
	return archive

def extract_files(archive):
	print("extract_files: started.")
	os.mkdir(EXTRACTED_FILES_DIR)
	for archived_file in archive:
		if archived_file.name in REGION_FILES:
			with open(path.join(EXTRACTED_FILES_DIR, archived_file.name), "wb") as stream:
				stream.write(archive.extractfile(archived_file).read())
	print("extract_files: done.")

def extract_timezones():
	print("extract_timezones: started.")
	timezones = []
	for region_file in REGION_FILES:
		print("extract_timezones: extracting timezones from \"%s\"." % region_file)
		timezones += Region.from_stream(
			Reader(region_file, open(path.join(EXTRACTED_FILES_DIR, region_file), 'r'))
		)
	print("extract_timezones: done.")
	return timezones

def update_timezones():
	print("update_timezones: started.")
	archive = download_archive()
	extract_files(archive)
	timezones = extract_timezones()
	inf_output = convert_timezones(timezones)
	print("update_timezones: writing inf output.")
	with open(CONVERTED_TIMEZONES_FILE, 'w') as stream:
		stream.write(inf_output)
	print("update_timezones: done.")

HELP_MSG = ''.join(
	"Format: ./update_timezones.py [--current_year year] [--tzdata_version tzdata] ",
	"[--no-dynamic-dst]\n",
	"The tool generates timezones information for registry. The behavior depends on whether dynamic ",
	"DST is allowed.\n",
	'\n',
	"Transition object is a transition from standard time to daylight saving time and back. It",
	"consists of standard time offset, daylight time transition date, daylight time offset, ",
	"standard time transition date. Standard time offset is offset from UT. Daylight time if offset ",
	"from standard time.\n",
	"Offset is hours, minutes and seconds to shift clocks for.\n",
	"Rule for a timezone is a \"Rule\" object, offset or None. <------ ?\n",
	'\n'
	"------ Behaviour if dynamic DST is allowed (NT ...)------\n",
	'\n',
	"------ Behaviour if dynamic DST is not allowed (NT ...) ------\n",
	"Timezone entry must contain either the transition object for every year from the current year ",
	"on (in the time of the timezone) or one transition object for the entire time."
)
def print_help():
	print("")
	print("")
	print("------ If dynamic DST is allowed ------")
	
	print(''.join(
		"At first, rules for a timezone are stripped according to the \"UNTIL\" column. Then the ",
		"convertation starts."
	))
	print(''.join(
		"Timezone entry should contain general rules (that have \"max\" year in IANA timezone ",
		"database). If dynamic DST is not allowed, those rules must cover all the time from current ",
		"moment on (because only one transition object can be stored). If there are no such rules, ",
		"timezone entry will tell the system that time saving is not needed. If these rules can be ",
		"converted only if dynamic DST is allowed and dynamic DST is disabled, the timezone will be",
		"skipped and the user will be notified. If these rules can be converted only if dynamic DST ",
		"is allowed and dynamic DST is enabled, there will be no time saving in the timezone entry, ",
		"but dynamic DST entries for the next 250 years."
	))
	print(''.join(
		"If dynamic dst is allowed, then timezone entry will have \"Dynamic DST\" subentry. For every ",
		"year between the minimum year mentioned and the maximum year mentioned (not including ",
		"\"max\" year rules as they are processed by the step above) the tool will convert rules for ",
		"that year into transition objects. If it fails to convert a year, the years before the ",
		"failed year will be ignored."
	))

def parse_args(args):
	item_to_argument_types = {
		"--no-dynamic-dst": (,),
		"--current_year": (int,),
		"--tzdata_version": (str,)
	}
	result = {}
	args = reversed(args)
	while args:
		item = args.pop()
		if item not in item_to_argument_types:
			raise WrongUsageException("unsupported argument or repeated usage.")
		item_argument_types = item_to_argument_types[item]
		del item_to_argument_types[item]
		item_arguments = []
		for argument_type in item_argument_types:
			item_argument.append(argument_type(args.pop()))
		result[item] = item_arguments
	return result

def get_parameter(parameters, parameter, index, default_value):
	if parameter in parameters:
		return parameters[parameter][index]
	else:
		return default_value


if __name__ == "__main__":
	parameters = None
	try:
		parameters = parse_args(sys.argv)
	except WrongUsageException as exc:
		print_help()
		print(exc)
		exit(1)
	tzdata_version = get_parameter(parameters, "--tzdata_version", 0, None)
	# current_year = get_parameter(parameters, "--current_utc_time", 0, timezone.utcnow().year)
	current_utc_time = timezone.utcnow() # Do we need the ability to specify current time?
	dynamic_dst_is_allowed = False # "--no-dynamic-dst" not in parameters
	# Latest is used by default.
	update_timezones(tzdata_version, current_utc_time, dynamic_dst_is_allowed)
