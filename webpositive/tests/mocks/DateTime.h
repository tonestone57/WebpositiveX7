/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef DATE_TIME_H
#define DATE_TIME_H

#include <SupportDefs.h>

class BMessage;

class BDate {
public:
	BDate() : fYear(0), fMonth(0), fDay(0) {}
	BDate(int year, int month, int day) : fYear(year), fMonth(month), fDay(day) {}

	int Year() const { return fYear; }
	int Month() const { return fMonth; }
	int Day() const { return fDay; }

	void AddDays(int days) {
		// Mock implementation: Very simple add
		fDay += days;
	}

	bool operator<(const BDate& other) const {
		if (fYear != other.fYear) return fYear < other.fYear;
		if (fMonth != other.fMonth) return fMonth < other.fMonth;
		return fDay < other.fDay;
	}

	bool operator==(const BDate& other) const {
		return fYear == other.fYear && fMonth == other.fMonth && fDay == other.fDay;
	}

    BString LongDayName() const { return "Monday"; }

private:
	int fYear;
	int fMonth;
	int fDay;
};

class BTime {
public:
	BTime() : fHour(0), fMinute(0), fSecond(0) {}
	BTime(int hour, int minute, int second) : fHour(hour), fMinute(minute), fSecond(second) {}

	bool operator<(const BTime& other) const {
		if (fHour != other.fHour) return fHour < other.fHour;
		if (fMinute != other.fMinute) return fMinute < other.fMinute;
		return fSecond < other.fSecond;
	}

	bool operator==(const BTime& other) const {
		return fHour == other.fHour && fMinute == other.fMinute && fSecond == other.fSecond;
	}

private:
	int fHour;
	int fMinute;
	int fSecond;
};

class BDateTime {
public:
	BDateTime() : fTimeValue(0) {}
	BDateTime(const BDate& date, const BTime& time) : fDate(date), fTime(time), fTimeValue(0) {}
	BDateTime(const BMessage* message) : fTimeValue(0) {}

	status_t Archive(BMessage* archive) const { return B_OK; }

	void SetDate(const BDate& date) { fDate = date; }
	void SetTime(const BTime& time) { fTime = time; }

	BDate Date() const { return fDate; }
	BDate& Date() { return fDate; } // Return ref for non-const
	BTime Time() const { return fTime; }

    // Add Time_t
    time_t Time_t() const { return fTimeValue; }
	void SetTime_t(time_t value) { fTimeValue = value; }

	bool operator<(const BDateTime& other) const {
		if (fDate < other.fDate) return true;
		if (other.fDate < fDate) return false;
		return fTime < other.fTime;
	}

	bool operator==(const BDateTime& other) const {
		return fDate == other.fDate && fTime == other.fTime;
	}

	bool operator!=(const BDateTime& other) const {
		return !(*this == other);
	}

	static BDateTime CurrentDateTime(int32 type) {
		return BDateTime(BDate(2024, 1, 1), BTime(12, 0, 0));
	}

private:
	BDate fDate;
	BTime fTime;
};

enum time_type {
	B_LOCAL_TIME,
	B_GMT_TIME
};

#endif // DATE_TIME_H
