#include <rtc.h>
#include <common.h>

char * weekday_map[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

int is_updating_rtc()
{
    outb(CMOS_ADDR, 0x0A);
    uint32_t status = inb(CMOS_DATA);
    return (status & 0x80);
}

uint8_t get_rtc_register(int reg_num)
{
    outb(CMOS_ADDR, reg_num);
    return inb(CMOS_DATA);
}

void set_rtc_register(uint16_t reg_num, uint8_t val) {
    outb(CMOS_ADDR, reg_num);
    outb(CMOS_DATA, val);
}

void rtc_read_datetime()
{
    while(is_updating_rtc());

    current_datetime.second = get_rtc_register(0x00);
    current_datetime.minute = get_rtc_register(0x02);
    current_datetime.hour = get_rtc_register(0x04);
    current_datetime.day = get_rtc_register(0x07);
    current_datetime.month = get_rtc_register(0x08);
    current_datetime.year = get_rtc_register(0x09);

    uint8_t registerB = get_rtc_register(0x0B);

    if (!(registerB & 0x04))
    {
        current_datetime.second = (current_datetime.second & 0x0F) + ((current_datetime.second / 16) * 10);
        current_datetime.minute = (current_datetime.minute & 0x0F) + ((current_datetime.minute / 16) * 10);
        current_datetime.hour = ( (current_datetime.hour & 0x0F) + (((current_datetime.hour & 0x70) / 16) * 10) ) | (current_datetime.hour & 0x80);
        current_datetime.day = (current_datetime.day & 0x0F) + ((current_datetime.day / 16) * 10);
        current_datetime.month = (current_datetime.month & 0x0F) + ((current_datetime.month / 16) * 10);
        current_datetime.year = (current_datetime.year & 0x0F) + ((current_datetime.year / 16) * 10);
    }
}

void rtc_write_datetime(datetime_t * dt)
{
    while(is_updating_rtc());
    set_rtc_register(0x00, dt->second);
    set_rtc_register(0x02, dt->minute);
    set_rtc_register(0x04, dt->hour);
    set_rtc_register(0x07, dt->day);
    set_rtc_register(0x08, dt->month);
    set_rtc_register(0x09, dt->year);
}

char * datetime_to_str(datetime_t * dt)
{
    char* ret = malloc(30);
    char* start = ret;
    char day[4];
    char hour[3];
    char min[3];
    char sec[3];

    memset(&day, 0x0, 4);
    memset(&hour, 0x0, 3);
    memset(&min, 0x0, 3);
    memset(&sec, 0x0, 3);

    char * weekday = weekday_map[get_weekday_from_date(dt)];
    strcpy(day, weekday);
    itoa(hour, dt->hour, 10);
    itoa(min, dt->minute, 10);
    itoa(sec, dt->second, 10);

    strcpy(ret, day);
    strcat(ret, " ");
    strcat(ret, hour);
    strcat(ret, ":");
    strcat(ret, min);
    strcat(ret, ":");
    strcat(ret, sec);
    strcat(ret, " PM");
    ret = start;
    return ret;
}

char * get_current_datetime_str()
{
    return datetime_to_str(&current_datetime);
}

int get_weekday_from_date(datetime_t * dt)
{
    char month_code_array[] = {0x0,0x3, 0x3, 0x6, 0x1, 0x4, 0x6, 0x2, 0x5, 0x0, 0x3, 0x5};
    char century_code_array[] = {0x4, 0x2, 0x0, 0x6, 0x4, 0x2, 0x0};    // Starting from 18 century

    dt->century = 21;
    int year_code = (dt->year + (dt->year / 4)) % 7;
    int month_code = month_code_array[dt->month - 1];
    int century_code = century_code_array[dt->century - 1 - 17];
    int leap_year_code = is_leap_year(dt->year, dt->month);

    int ret = (year_code + month_code + century_code + dt->day - leap_year_code) % 7;
    return ret;
}

int is_leap_year(int year, int month)
{
    if(year % 4 == 0 && (month == 1 || month == 2))
        return 1;
    return 0;
}

void rtc_init()
{
    /*
       current_datetime.century = 21;
       current_datetime.year = 16;
       current_datetime.month = 1;
       current_datetime.day = 1;
       current_datetime.hour = 0;
       current_datetime.minute = 0;
       current_datetime.second = 0;
       rtc_write_datetime(&current_datetime);
       */
    rtc_read_datetime();
}
