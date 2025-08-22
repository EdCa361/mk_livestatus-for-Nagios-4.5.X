# mk_livestatus-for-Nagios-4.5.X
Livestatus module for Nagios 4.5.X


This repository is a fork and improvement of the original `mk_livestatus` project found at [Expensify/mk_livestatus](https://github.com/Expensify/mk_livestatus/tree/main).

This enhanced version includes the following key updates:

1.  **Updated Header Files for Nagios 4:** The header files (`.h`) in the `nagios4` folder have been updated. They are directly sourced from Nagios Core 4.5.9, specifically from the `lib`, `include`, and `base` directories. These are the standard, default header files.
2.  **Resolved Log Error:** A recurring error that caused a log entry "livestatus: Timeperiod cache not updated, there are no timeperiods (yet)" every minute has been fixed.

## Installation

The installation instructions for Linux are as follows:

```bash
./configure
make
make install
