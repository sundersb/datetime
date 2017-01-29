# DateTime class
Quick and small C++ date and time class

Known C++ &lt;ctime&gt; date and time structures have disappointing limitations which have driven me to make my own class.

For instance, time_t is not crossplatform-consistant and can't keep dates before 1970. Struct tm keeps dates from 1900 only.

Modern C++ versions suggest more appropriate ways to track date and time. But what to do with, say, Visual Studio 2010? There are known libraries dealing with the subject but linking huge dependencies just for dates is not a stright-forward way. 

DateTime class is quick and small: it contains only one long long field which keeps number of milliseconds from the start of January the first of the first year (by Gregorian calendar). Instances may be initialized from a standard string, time_t or struct tm. Likewise it may be explicitly converted back to those data types.

You can apply some arythmetics to DateTime: increase or decrease dates by a certain amount of days, months, years, compare DateTime values etc. DateTime class is very helpful when you are working with relational databases: SQLite, PostgreSQL, MS SQL...

Just a simple example:

    DateTime now;
    now.setNow();

    now.incMonth(20);
    cout << now.formatDateTime() << endl;

    now.fromUTC();
    cout << now.formatDateTime() << endl;

    now.set("2017-01-28 22:12:50");

## Some disclaimer
Timezone is a tricky business and I can't say how fromUTC() and toUTC() will behave on different platforms. In detecting timezone gmtime_s() is used which is platform specific (I use Visual Studio 2010 Express as mentioned above). But you can easily fix that.

Enjoy!

Alexander Belkov
