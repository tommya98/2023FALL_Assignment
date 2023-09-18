#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>

struct korTime_t TimevalToKorTime(struct timeval time);
int isLeapYear(int year);
int countLeapYear(int year);
int getMonthFromPastDay(int pastDay, int year);
int getDayFromPastDay(int pastDay, int year);

struct korTime_t
{
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
};

int main(void)
{
  char *buf1 = "20181536 엄석훈\n";
  char buf2[100];
  int leapYearNum;
  struct timeval time;
  struct korTime_t curTime;

  gettimeofday(&time, NULL);
  leapYearNum = countLeapYear(1970 + time.tv_sec / 31536000);
  time.tv_sec -= 86400 * (leapYearNum - 1);
  time.tv_sec += 32400;
  curTime = TimevalToKorTime(time);
  sprintf(buf2, "%d년 %d월 %d일 %d시 %d분 %d초", curTime.year, curTime.month, curTime.day, curTime.hour, curTime.minute, curTime.second);

  int fd = open("./result.txt", O_WRONLY | O_CREAT, 0666);
  write(fd, buf1, strlen(buf1));
  write(fd, buf2, strlen(buf2));
  close(fd);

  return 0;
}

struct korTime_t TimevalToKorTime(struct timeval time)
{
  struct korTime_t korTime;
  int pastDay = ((time.tv_sec / 86400) % 365);

  korTime.year = 1970 + time.tv_sec / 31536000;
  korTime.month = getMonthFromPastDay(pastDay, korTime.year);
  korTime.day = getDayFromPastDay(pastDay, korTime.year);
  korTime.hour = ((time.tv_sec % 86400) / 3600);
  korTime.minute = (((time.tv_sec % 86400) % 3600) / 60);
  korTime.second = (((time.tv_sec % 86400) % 3600) % 60);

  return korTime;
}

int isLeapYear(int year)
{
  if (year % 4 == 0 && year % 100 != 0 || year % 400 == 0)
    return 1;

  return 0;
}

int countLeapYear(int year)
{
  int sum = 0;

  for (int i = 1970; i <= year; i++)
  {
    sum += isLeapYear(i);
  }

  return sum;
}

int getMonthFromPastDay(int pastDay, int year)
{
  int isLeap = isLeapYear(year);

  if (isLeap == 0)
  {
    if (pastDay <= 31)
    {
      return 1;
    }
    else if (pastDay <= 59)
    {
      return 2;
    }
    else if (pastDay <= 90)
    {
      return 3;
    }
    else if (pastDay <= 120)
    {
      return 4;
    }
    else if (pastDay <= 151)
    {
      return 5;
    }
    else if (pastDay <= 181)
    {
      return 6;
    }
    else if (pastDay <= 212)
    {
      return 7;
    }
    else if (pastDay <= 243)
    {
      return 8;
    }
    else if (pastDay <= 273)
    {
      return 9;
    }
    else if (pastDay <= 304)
    {
      return 10;
    }
    else if (pastDay <= 334)
    {
      return 11;
    }
    else
    {
      return 12;
    }
  }

  if (pastDay <= 31)
  {
    return 1;
  }
  else if (pastDay <= 60)
  {
    return 2;
  }
  else if (pastDay <= 91)
  {
    return 3;
  }
  else if (pastDay <= 121)
  {
    return 4;
  }
  else if (pastDay <= 152)
  {
    return 5;
  }
  else if (pastDay <= 182)
  {
    return 6;
  }
  else if (pastDay <= 213)
  {
    return 7;
  }
  else if (pastDay <= 244)
  {
    return 8;
  }
  else if (pastDay <= 274)
  {
    return 9;
  }
  else if (pastDay <= 305)
  {
    return 10;
  }
  else if (pastDay <= 335)
  {
    return 11;
  }
  else
  {
    return 12;
  }
}

int getDayFromPastDay(int pastDay, int year)
{
  int isLeap = isLeapYear(year);
  int month = getMonthFromPastDay(pastDay, year);

  if (isLeap == 0)
  {
    if (month == 1)
    {
      return pastDay;
    }
    else if (month == 2)
    {
      return pastDay - 31;
    }
    else if (month == 3)
    {
      return pastDay - 59;
    }
    else if (month == 4)
    {
      return pastDay - 90;
    }
    else if (month == 5)
    {
      return pastDay - 120;
    }
    else if (month == 6)
    {
      return pastDay - 151;
    }
    else if (month == 7)
    {
      return pastDay - 181;
    }
    else if (month == 8)
    {
      return pastDay - 212;
    }
    else if (month == 9)
    {
      return pastDay - 243;
    }
    else if (month == 10)
    {
      return pastDay - 273;
    }
    else if (month == 11)
    {
      return pastDay - 304;
    }
    else
    {
      return pastDay - 334;
    }
  }

  if (month == 1)
  {
    return pastDay;
  }
  else if (month == 2)
  {
    return pastDay - 31;
  }
  else if (month == 3)
  {
    return pastDay - 60;
  }
  else if (month == 4)
  {
    return pastDay - 91;
  }
  else if (month == 5)
  {
    return pastDay - 121;
  }
  else if (month == 6)
  {
    return pastDay - 152;
  }
  else if (month == 7)
  {
    return pastDay - 182;
  }
  else if (month == 8)
  {
    return pastDay - 213;
  }
  else if (month == 9)
  {
    return pastDay - 244;
  }
  else if (month == 10)
  {
    return pastDay - 274;
  }
  else if (month == 11)
  {
    return pastDay - 305;
  }
  else
  {
    return pastDay - 335;
  }

  return 0;
}