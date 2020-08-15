UpdateHub embedded Firmware Over-The-Air (FOTA) sample
######################################################

Overview
********

UpdateHub is an enterprise-grade solution which makes it simple to remotely
update all your embedded devices.  It handles all aspects related to sending
Firmware Over-the-Air (FOTA) updates with maximum security and efficiency,
while you focus on adding value to your product.  It is possible to read more
about at `docs.updatehub.io`_.

This sample shows how to use UpdateHub in both a polling and manual update
mode.

Polling mode runs automatically on a predefined period, probing the server
for updates and installing them without requiring user intervention.  You
can access the sample source code for this mode updatehub_polling.

Manual mode requires the user to call the server probe and then, if there is
an available update, also requires the user to decide if it is appropriate to
update now or later.  You can access the sample source code for this mode
updatehub_manual.

Caveats
*******

* The Zephyr port of ``UpdateHub`` is configured to run this sample on a
  :ref:`Freedom-K64F <frdm_k64f>` MCU by default.  The application should
  build and run for other platforms with offer support internet
  connection.  Some platforms need some modification.  Overlay files would
  be needed to support BLE 6lowpan, 802.15.4 or OpenThread configurations
  as well as the understanding that most other connectivity options would
  require an edge gateway of some sort (Border Router, etc).

* The MCUboot bootloader is required for ``UpdateHub`` to function properly.
  More information about the Device Firmware Upgrade subsystem and MCUboot
  can be found in :ref:`mcuboot`.

* ``UpdateHub`` acts as a service on Zephyr. It heavily depends on Zephyr
  sub-systems using CoAP over UDP.

* ``UpdateHub`` officialy support frdm_k64f board with ethernet, wifi and
  modem connectivity.


Building and Running
********************

The below steps describe how to build and run the ``UpdateHub`` sample in
Zephyr. Where examples are given, it is assumed the sample is being built for
the Freedom-K64F Development Kit (``BOARD=frdm_k64f``). Make sure your are
outside on the zephyr project root on all steps.  This allows contruct and run
everything from a common place.

.. code-block:: console

    ls
    bootloader  modules  tools  zephyr

Step 1: Build/Flash MCUboot
===========================

Build MCUboot by following the instructions in the :ref:`mcuboot` documentation
page.  Flash the resulting image file to the :ref:`mcuboot_partitions` of the
flash memory. This can be done in multiple ways, but the most simple one would
be using west.

.. zephyr-app-commands::
    :zephyr-app: bootloader/mcuboot/boot/zephyr
    :board: frdm_k64f
    :build-dir: mcuboot
    :goals: build flash

Step 2: Start the updatehub Community Edition
=============================================

By default, the updatehub application is set to start on the UpdateHub Cloud.
For more details on how to use the UpdateHub Cloud please refer to the
documentation on `updatehub.io`_.  The UpdateHub Cloud has the option to use
CoAPS/DTLS or not.  If you want to use the CoAPS/DTLS, simply add the
``overlay-dtls.conf`` before building.  You must only use the provided
certificate for the test example.  Otherwise, you should create a new
certificate using these commands on another terminal ``terminal 2``:

.. code-block:: console

    openssl genrsa -out privkey.pem 512
    openssl req -new -x509 -key privkey.pem -out servercert.pem

The cert and private key that will be embedded into ``certificates.h`` in
your application, can be generated like this:

.. code-block:: console

    openssl x509 -in servercert.pem -outform DER -out servercert.der
    openssl pkcs8 -topk8 -inform PEM -outform DER -nocrypt -in privkey.pem
      -out privkey.der

If you would like to use your own server, the steps below explain how
updatehub works with updatehub-ce running, started by the following Docker
command:

.. code-block:: console

    docker run -it -p 8080:8080 -p 5683:5683/udp --rm
      updatehub/updatehub-ce:latest

Using this server, create your own ``overlay-prj.conf``, setting the
option :option:`CONFIG_UPDATEHUB_SERVER` with your local IP address and
the option :option:`CONFIG_UPDATEHUB_CE` with true.  If you're using the
polling mode on UpdateHub, you'll also need to set the option
:option:`CONFIG_UPDATEHUB_POLL_INTERVAL` with the polling period of your
preference, remembering that the limit is between 0 and 43200 minutes
(30 days).  This server does not use DTLS, so you must not add
``overlay-dtls.config``.  This sample uses IPv4 by default, but you can
use IPv6 by enabling IPv6 and configuring your IP address.

Step 3: Build UpdateHub Sample
==============================

This show how to build ``UpdateHub`` for some technologies.  To do it,
``UpdateHub`` define base configuration at ``prj.conf`` and extends or drop
configurations using overlay configs. Go back to ``terminal 1`` and run any
build.

Step 3.1: Build for Ethernet
----------------------------

The ethernet depends only from base configuration.

.. zephyr-app-commands::
    :zephyr-app: zephyr/samples/subsys/mgmt/updatehub
    :board: frdm_k64f
    :conf: "prj.conf overlay-prj.conf"
    :goals: build

Step 3.2: Build for WiFi
------------------------

For WiFi, it needs add ``overlay-wifi.conf``.  Here a shield provides WiFi
connectivity using, for instance, arduino headers.  See :ref:`module_esp_8266`
for details.

.. zephyr-app-commands::
    :zephyr-app: zephyr/samples/subsys/mgmt/updatehub
    :board: frdm_k64f
    :conf: "prj.conf overlay-prj.conf" -DOVERLAY_CONFIG=overlay-wifi.conf
    :shield: esp_8266_arduino
    :goals: build

Step 3.3: Build for Modem
-------------------------

Modem needs add ``overlay-modem.conf``.  Now, a DTC overlay file is used to
configure the glue between the modem and an arduino headers.  The modem config
uses PPP over GSM modem, see :ref:`gsm-modem-sample`.

.. zephyr-app-commands::
    :zephyr-app: zephyr/samples/subsys/mgmt/updatehub
    :board: frdm_k64f
    :conf: "prj.conf overlay-prj.conf" -DOVERLAY_CONFIG=overlay-modem.conf \
      -DDTC_OVERLAY_FILE=arduino.overlay
    :goals: build

Step 4: Sign the first image
============================

From this section onwards you use a binary (``.bin``) image format.  This
depends on how the board is configured. There is a (``.hex``) image format
variation too.

Step 4.1: Using MCUBoot
-----------------------

Using MCUboot's :file:`imgtool.py` script, sign the :file:`zephyr.bin`
file you built in Step 3.

.. code-block:: console

  ~/bootloader/mcuboot/scripts/imgtool.py sign \
	    --key ~/bootloader/mcuboot/root-rsa-2048.pem \
	    --align 8 \
	    --version 1.0.0 \
	    --header-size 0x200 \
	    --slot-size <image-slot-size> \
	    --pad \
	    <path-to-zephyr.bin> build/zephyr/zephyr.signed.bin

Step 4.2: Using West
--------------------

West invoke MCUboot filling default parameters.  It is simpler than call
MCUBoot imgtool directly but is less flexible.

.. code-block:: console

  west sign -t imgtool -- --key bootloader/mcuboot/root-rsa-2048.pem

Both ways create an image file called :file:`build/zephyr/zephyr.signed.bin`
in the current directory.

Step 5: Flash the first image
=============================

Upload the :file:`zephyr.signed.bin` file from Step 4 to image slot-0 of your
board.  The location of image slot-0 varies by board, as described in
:ref:`mcuboot_partitions`.  For the frdm_k64f, slot-0 is located at address
``0xc000``.

Step 5.1: Using PyOCD
---------------------

.. code-block:: console

    sudo pyocd-flashtool <path-to-signed.bin>

Step 5.2: Using West
--------------------

.. code-block:: console

    west flash --bin-file build/zephyr/zephyr.signed.bin

.. note:: Command variation to flash a ``hex`` file:
    ``west flash --hex-file build/zephyr/zephyr.signed.hex``

At this point you can access a third terminal ``terminal 3`` to check if image
is running:

.. code-block:: console

    minicom -D /dev/ttyACM0


Step 6: Signing the test image
==============================

For the update to be correctly validated on the server, you must need sign the
(``bin/hex``) image, piping the output to another file.

.. code-block:: console

   ~/bootloader/mcuboot/scripts/imgtool.py sign \
	    --key ~/bootloader/mcuboot/root-rsa-2048.pem \
	    --align 8 \
	    --version 2.0.0 \
	    --header-size 0x200 \
	    --slot-size <image-slot-size> \
	    --pad \
            <path-to-zephyr.bin> build/zephyr/zephyr.signed_v2.bin


Step 7: Create a package with UpdateHub Utilities (uhu)
=======================================================

First, install UpdateHub Utilities (``uhu``) on your system, using:

.. code-block:: console

    pip3 install --user uhu

After installing uhu you will need to set the ``product-uid``:

.. code-block:: console

    uhu product use "e4d37cfe6ec48a2d069cc0bbb8b078677e9a0d8df3a027c4d8ea131130c4265f"

Then, add the package and its mode (``zephyr``):

.. code-block:: console

    uhu package add build/zephyr/zephyr.signed_v2.bin -m zephyr

Then inform what ``version`` this image is:

.. code-block:: console

   uhu package version 2.0.0

And finally you can build the package by running:

.. code-block:: console

    uhu package archive --output build/zephyr/<name-of-package>.pkg


Step 8: Add the package to server
=================================

Now, add the package to the updatehub-ce by, opening your browser to
the server URL, ``<your-ip-address>:8080``, and logging into the server using
``admin`` as the login and password by default.  After logging in, click on
the package menu, then ``UPLOAD PACKAGE``, and select the package built in
step 7.

Step 9: Register device on server
=================================

Register your device at updatehub-ce by using a terminal session on the system
where you were debugging the board ``terminal 3``, and type the following
command:

.. code-block:: console

    updatehub run

If everything is alright, it will print on the screen ``No update available``.

.. note::
    The message ``Could not receive data`` means that your UpdateHub-CE server
    was not reached for some reason.  The most common cases are server down,
    missing routes and forget to change the content of ``overlay-prj.conf``
    file.


Step 10: Create a rollout
=========================

In the browser where the updatehub-ce is open, click on ``menu Rollout``
and then ``CREATE ROLLOUT``.  Select the version of the package that you added
in step 9.  With that, the update is published, and the server is ready to
accept update requests.

Step 11: Run the update
=======================

Back in the terminal session that you used for debugging the board, type the
following command:

.. code-block:: console

    updatehub run

And then wait.  The board will probe the server, check if there are any new
updates, and then download the update package you've just created. If
everything goes fine the message ``Image flashed successfully, you can reboot
now`` will be printed on the terminal.

Step 12: Reboot the system
==========================

In the terminal you used for debugging the board, type the following command:

.. code-block:: console

    kernel reboot cold

Your board will reboot and then start with the new image.  After rebooting,
the board will automatically ping the server again and the message ``No update
available`` will be printed on the terminal.

.. _updatehub.io: https://updatehub.io
.. _docs.updatehub.io: https://docs.updatehub.io/
