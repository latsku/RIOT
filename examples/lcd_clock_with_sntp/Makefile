# name of your application
APPLICATION = lcd_clock_with_sntp

# If no BOARD is found in the environment, use this default:
BOARD ?= nucleo-f401

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

BOARD_INSUFFICIENT_MEMORY := airfy-beacon chronos msb-430 msb-430h nrf51dongle \
                          nrf6310 nucleo-f334 pca10000 pca10005 spark-core \
                          stm32f0discovery telosb weio wsn430-v1_3b wsn430-v1_4 \
                          yunjia-nrf51822 z1

# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += gnrc_netif_default
USEMODULE += auto_init_gnrc_netif
# Specify the mandatory networking modules for IPv6 and UDP
USEMODULE += gnrc_conn_udp
# Add a routing protocol
USEMODULE += gnrc_rpl
# Additional networking modules that can be dropped if not needed
USEMODULE += gnrc_icmpv6_echo
# Add also the shell, some shell commands
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

# LCD display driver
USEMODULE += hd44780
USEMODULE += timex


# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -DDEVELHELP

# Configuration to enable PoEll-i Nucleo ethernet adapter.
USEMODULE += encx24j600
USEMODULE += gnrc_netdev2
CFLAGS += -DENCX24J600_SPI=SPI_0
CFLAGS += -DENCX24J600_CS="GPIO_PIN(PORT_C,10)" -DENCX24J600_INT="GPIO_PIN(PORT_D,2)"


FEATURES_REQUIRED += periph_rtc


# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
