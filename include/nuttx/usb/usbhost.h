/************************************************************************************
 * include/nuttx/usb/usbhost.h
 *
 *   Copyright (C) 2010-2015 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * References:
 *   "Universal Serial Bus Mass Storage Class, Specification Overview,"
 *   Revision 1.2,  USB Implementer's Forum, June 23, 2003.
 *
 *   "Universal Serial Bus Mass Storage Class, Bulk-Only Transport,"
 *   Revision 1.0, USB Implementer's Forum, September 31, 1999.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ************************************************************************************/

#ifndef __INCLUDE_NUTTX_USB_USBHOST_H
#define __INCLUDE_NUTTX_USB_USBHOST_H

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

#include <nuttx/usb/usbhost_devaddr.h>

/************************************************************************************
 * Pre-processor Definitions
 ************************************************************************************/

/************************************************************************************
 * Name: ROOTHUB
 *
 * Description:
 *   Check if a hub instance is the root hub.
 *
 ************************************************************************************/

#define ROOTHUB(hub) ((hub)->parent == NULL)

/************************************************************************************
 * Name: CLASS_CREATE
 *
 * Description:
 *   This macro will call the create() method of struct usbhost_registry_s.  The create()
 *   method is a callback into the class implementation.  It is used to (1) create
 *   a new instance of the USB host class state and to (2) bind a USB host driver
 *   "session" to the class instance.  Use of this create() method will support
 *   environments where there may be multiple USB ports and multiple USB devices
 *   simultaneously connected.
 *
 * Input Parameters:
 *   reg - The USB host class registry entry previously obtained from a call to
 *     usbhost_findclass().
 *   hport - The hub hat manages the new class instance.
 *   id - In the case where the device supports multiple base classes, subclasses, or
 *     protocols, this specifies which to configure for.
 *
 * Returned Values:
 *   On success, this function will return a non-NULL instance of struct
 *   usbhost_class_s that can be used by the USB host driver to communicate with the
 *   USB host class.  NULL is returned on failure; this function will fail only if
 *   the drvr input parameter is NULL or if there are insufficient resources to
 *   create another USB host class instance.
 *
 * Assumptions:
 *   If this function is called from an interrupt handler, it will be unable to
 *   allocate memory and CONFIG_USBHOST_NPREALLOC should be defined to be a value
 *   greater than zero specify a number of pre-allocated class structures.
 *
 ************************************************************************************/

#define CLASS_CREATE(reg,hport,id) ((reg)->create(hport,id))

/************************************************************************************
 * Name: CLASS_CONNECT
 *
 * Description:
 *   This macro will call the connect() method of struct usbhost_class_s.  This
 *   method is a callback into the devclass implementation.  It is used to provide the
 *   device's configuration descriptor to the devclass so that the devclass may initialize
 *   properly
 *
 * Input Parameters:
 *   devclass - The USB host class entry previously obtained from a call to create().
 *   configdesc - A pointer to a uint8_t buffer container the configuration descriptor.
 *   desclen - The length in bytes of the configuration descriptor.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 *   NOTE that the class instance remains valid upon return with a failure.  It is
 *   the responsibility of the higher level enumeration logic to call
 *   CLASS_DISCONNECTED to free up the class driver resources.
 *
 * Assumptions:
 *   - This function is probably called on the same thread that called the driver
 *     enumerate() method. This function will *not* be called from an interrupt
 *     handler.
 *   - If this function returns an error, the USB host controller driver
 *     must call to DISCONNECTED method to recover from the error
 *
 ************************************************************************************/

#define CLASS_CONNECT(devclass,configdesc,desclen) \
  ((devclass)->connect(devclass,configdesc,desclen))

/************************************************************************************
 * Name: CLASS_DISCONNECTED
 *
 * Description:
 *   This macro will call the disconnected() method of struct usbhost_class_s.  This
 *   method is a callback into the class implementation.  It is used to inform the
 *   class that the USB device has been disconnected.
 *
 * Input Parameters:
 *   devclass - The USB host class entry previously obtained from a call to create().
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   This function will *not* be called from an interrupt handler.
 *
 ************************************************************************************/

#define CLASS_DISCONNECTED(devclass) ((devclass)->disconnected(devclass))

/*******************************************************************************
 * Name: CONN_WAIT
 *
 * Description:
 *   Wait for a device to be connected or disconnected to/from a root hub port.
 *
 * Input Parameters:
 *   conn - The USB host connection instance obtained as a parameter from the call to
 *      the USB driver initialization logic.
 *   connected - A pointer to an array of n boolean values corresponding to
 *      root hubs 1 through n.  For each boolean value: TRUE: Wait for a device
 *      to be connected on the root hub; FALSE: wait for device to be
 *      disconnected from the root hub.
 *
 * Returned Values:
 *   And index [0..(n-1)} corresponding to the root hub port number {1..n} is
 *   returned when a device in connected or disconnected. This function will not
 *   return until either (1) a device is connected or disconnect to/from any
 *   root hub port or until (2) some failure occurs.  On a failure, a negated
 *   errno value is returned indicating the nature of the failure
 *
 * Assumptions:
 *   - Called from a single thread so no mutual exclusion is required.
 *   - Never called from an interrupt handler.
 *
 *******************************************************************************/

#define CONN_WAIT(conn, connected) ((conn)->wait(conn,connected))

/************************************************************************************
 * Name: CONN_ENUMERATE
 *
 * Description:
 *   Enumerate the connected device.  As part of this enumeration process,
 *   the driver will (1) get the device's configuration descriptor, (2)
 *   extract the class ID info from the configuration descriptor, (3) call
 *   usbhost_findclass() to find the class that supports this device, (4)
 *   call the create() method on the struct usbhost_registry_s interface
 *   to get a class instance, and finally (5) call the connect() method
 *   of the struct usbhost_class_s interface.  After that, the class is in
 *   charge of the sequence of operations.
 *
 * Input Parameters:
 *   conn - The USB host connection instance obtained as a parameter from the call to
 *      the USB driver initialization logic.
 *   rphndx - Root hub port index.  0-(n-1) corresponds to root hub port 1-n.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   This function will *not* be called from an interrupt handler.
 *
 ************************************************************************************/

#define CONN_ENUMERATE(conn,rhpndx) ((conn)->enumerate(conn,rhpndx))

/************************************************************************************
 * Name: DRVR_EP0CONFIGURE
 *
 * Description:
 *   Configure endpoint 0.  This method is normally used internally by the
 *   enumerate() method but is made available at the interface to support
 *   an external implementation of the enumeration logic.
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *   ep0 - The (opaque) EP0 endpoint instance
 *   funcaddr - The USB address of the function containing the endpoint that EP0
 *     controls
 *   mps (maxpacketsize) - The maximum number of bytes that can be sent to or
 *    received from the endpoint in a single data packet
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   This function will *not* be called from an interrupt handler.
 *
 ************************************************************************************/

#define DRVR_EP0CONFIGURE(drvr,ep0,funcaddr,mps) \
  ((drvr)->ep0configure(drvr,ep0,funcaddr,mps))

/************************************************************************************
 * Name: DRVR_GETDEVINFO
 *
 * Description:
 *   Get information about the connected device.
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *   devinfo - A pointer to memory provided by the caller in which to return the
 *      device information.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   This function will *not* be called from an interrupt handler.
 *
 ************************************************************************************/

#define DRVR_GETDEVINFO(drvr,devinfo) ((drvr)->getdevinfo(drvr,devinfo))

/************************************************************************************
 * Name: DRVR_EPALLOC
 *
 * Description:
 *   Allocate and configure one endpoint.
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *   epdesc - Describes the endpoint to be allocated.
 *   ep - A memory location provided by the caller in which to receive the
 *      allocated endpoint descriptor.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   This function will *not* be called from an interrupt handler.
 *
 ************************************************************************************/

#define DRVR_EPALLOC(drvr,epdesc,ep) ((drvr)->epalloc(drvr,epdesc,ep))

/************************************************************************************
 * Name: DRVR_EPFREE
 *
 * Description:
 *   Free and endpoint previously allocated by DRVR_EPALLOC.
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *   ep - The endpoint to be freed.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   This function will *not* be called from an interrupt handler.
 *
 ************************************************************************************/

#define DRVR_EPFREE(drvr,ep) ((drvr)->epfree(drvr,ep))

/************************************************************************************
 * Name: DRVR_ALLOC
 *
 * Description:
 *   Some hardware supports special memory in which request and descriptor data can
 *   be accessed more efficiently.  This method provides a mechanism to allocate
 *   the request/descriptor memory.  If the underlying hardware does not support
 *   such "special" memory, this functions may simply map to kmm_malloc.
 *
 *   This interface was optimized under a particular assumption.  It was assumed
 *   that the driver maintains a pool of small, pre-allocated buffers for descriptor
 *   traffic.  NOTE that size is not an input, but an output:  The size of the
 *   pre-allocated buffer is returned.
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *   buffer - The address of a memory location provided by the caller in which to
 *     return the allocated buffer memory address.
 *   maxlen - The address of a memory location provided by the caller in which to
 *     return the maximum size of the allocated buffer memory.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   This function will *not* be called from an interrupt handler.
 *
 ************************************************************************************/

#define DRVR_ALLOC(drvr,buffer,maxlen) ((drvr)->alloc(drvr,buffer,maxlen))

/************************************************************************************
 * Name: DRVR_FREE
 *
 * Description:
 *   Some hardware supports special memory in which request and descriptor data can
 *   be accessed more efficiently.  This method provides a mechanism to free that
 *   request/descriptor memory.  If the underlying hardware does not support
 *   such "special" memory, this functions may simply map to kmm_free().
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *   buffer - The address of the allocated buffer memory to be freed.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   This function will *not* be called from an interrupt handler.
 *
 ************************************************************************************/

#define DRVR_FREE(drvr,buffer) ((drvr)->free(drvr,buffer))

/************************************************************************************
 * Name: DRVR_IOALLOC
 *
 * Description:
 *   Some hardware supports special memory in which larger IO buffers can
 *   be accessed more efficiently.  This method provides a mechanism to allocate
 *   the request/descriptor memory.  If the underlying hardware does not support
 *   such "special" memory, this functions may simply map to kmm_malloc.
 *
 *   This interface differs from DRVR_ALLOC in that the buffers are variable-sized.
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *   buffer - The address of a memory location provided by the caller in which to
 *     return the allocated buffer memory address.
 *   buflen - The size of the buffer required.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   This function will *not* be called from an interrupt handler.
 *
 ************************************************************************************/

#define DRVR_IOALLOC(drvr,buffer,buflen) ((drvr)->ioalloc(drvr,buffer,buflen))

/************************************************************************************
 * Name: DRVR_IOFREE
 *
 * Description:
 *   Some hardware supports special memory in which IO data can  be accessed more
 *   efficiently.  This method provides a mechanism to free that IO buffer
 *   memory.  If the underlying hardware does not support such "special" memory,
 *   this functions may simply map to kmm_free().
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *   buffer - The address of the allocated buffer memory to be freed.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   This function will *not* be called from an interrupt handler.
 *
 ************************************************************************************/

#define DRVR_IOFREE(drvr,buffer) ((drvr)->iofree(drvr,buffer))

/************************************************************************************
 * Name: DRVR_CTRLIN and DRVR_CTRLOUT
 *
 * Description:
 *   Process a IN or OUT request on the control endpoint.  These methods
 *   will enqueue the request and wait for it to complete.  Only one transfer may be
 *   queued; Neither these methods nor the transfer() method can be called again
 *   until the control transfer functions returns.
 *
 *   These are blocking methods; these functions will not return until the
 *   control transfer has completed.
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *   ep0 - The control endpoint to send/receive the control request.
 *   req - Describes the request to be sent.  This request must lie in memory
 *      created by DRVR_ALLOC.
 *   buffer - A buffer used for sending the request and for returning any
 *     responses.  This buffer must be large enough to hold the length value
 *     in the request description. buffer must have been allocated using DRVR_ALLOC.
 *
 *   NOTE: On an IN transaction, req and buffer may refer to the same allocated
 *   memory.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   This function will *not* be called from an interrupt handler.
 *
 ************************************************************************************/

#define DRVR_CTRLIN(drvr,ep0,req,buffer)  ((drvr)->ctrlin(drvr,ep0,req,buffer))
#define DRVR_CTRLOUT(drvr,ep0,req,buffer) ((drvr)->ctrlout(drvr,ep0,req,buffer))

/************************************************************************************
 * Name: DRVR_TRANSFER and DRVR_ASYNCH
 *
 * Description:
 *   Process a request to handle a transfer descriptor.  This method will
 *   enqueue the transfer request and rwait for it to complete.  Only one transfer may
 *   be queued; Neither this method nor the ctrlin or ctrlout methods can be called
 *   again until this function returns.
 *
 * - 'transfer' is a blocking method; this method will not return until the
 *   transfer has completed.
 * - 'asynch' will return immediately.  When the transfer completes, the
 *    semaphore will be posted.
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *   ep - The IN or OUT endpoint descriptor for the device endpoint on which to
 *      perform the transfer.
 *   buffer - A buffer containing the data to be sent (OUT endpoint) or received
 *     (IN endpoint).  buffer must have been allocated using DRVR_ALLOC
 *   buflen - The length of the data to be sent or received.
 *   callback - This function will be called when the transfer completes ('asynch'
 *     only).
 *   arg - The arbitrary parameter that will be passed to the callback function
 *     when the transfer completes ('asynch' only).
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure:
 *
 *     EAGAIN - If devices NAKs the transfer (or NYET or other error where
 *              it may be appropriate to restart the entire transaction).
 *     EPERM  - If the endpoint stalls
 *     EIO    - On a TX or data toggle error
 *     EPIPE  - Overrun errors
 *
 * Assumptions:
 *   This function will *not* be called from an interrupt handler.
 *
 ************************************************************************************/

#define DRVR_TRANSFER(drvr,ep,buffer,buflen) \
  ((drvr)->transfer(drvr,ep,buffer,buflen))

#ifdef CONFIG_USBHOST_ASYNCH
#define DRVR_ASYNCH(drvr,ep,buffer,buflen,callback,arg) \
  ((drvr)->asynch(drvr,ep,buffer,buflen,callback,arg))
#endif

/************************************************************************************
 * Name: DRVR_DISCONNECT
 *
 * Description:
 *   Called by the class when an error occurs and driver has been disconnected.
 *   The USB host driver should discard the handle to the class instance (it is
 *   stale) and not attempt any further interaction with the class driver instance
 *   (until a new instance is received from the create() method).  The driver
 *   should not called the class' disconnected() method.
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *
 * Returned Values:
 *   None
 *
 * Assumptions:
 *   This function will *not* be called from an interrupt handler.
 *
 ************************************************************************************/

#define DRVR_DISCONNECT(drvr) ((drvr)->disconnect(drvr))

/************************************************************************************
 * Public Types
 ************************************************************************************/

/* This struct contains all of the information that is needed to associate a device
 * this is connected via a USB port to a class.
 */

struct usbhost_id_s
{
  uint8_t  base;     /* Base device class code (see USB_CLASS_* defines in usb.h) */
  uint8_t  subclass; /* Sub-class, depends on base class. Eg., See USBMSC_SUBCLASS_* */
  uint8_t  proto;    /* Protocol, depends on base class. Eg., See USBMSC_PROTO_* */
  uint16_t vid;      /* Vendor ID (for vendor/product specific devices) */
  uint16_t pid;      /* Product ID (for vendor/product specific devices) */
};

/* The struct usbhost_registry_s type describes information that is kept in the the
 * USB host registry.  USB host class implementations register this information so
 * that USB host drivers can later find the class that matches the device that is
 * connected to the USB port.
 */

struct usbhost_hubport_s;    /* Forward reference to the hub state structure */
struct usbhost_class_s;  /* Forward reference to the class state structure */
struct usbhost_registry_s
{
  /* This field is used to implement a singly-link registry structure.  Because of
   * the presence of this link, provides of struct usbhost_registry_s instances must
   * provide those instances in write-able memory (RAM).
   */

  struct usbhost_registry_s *flink;

  /* This is a callback into the class implementation.  It is used to (1) create
   * a new instance of the USB host class state and to (2) bind a USB host driver
   * "session" to the class instance.  Use of this create() method will support
   * environments where there may be multiple USB ports and multiple USB devices
   * simultaneously connected (see the CLASS_CREATE() macro above).
   */

  FAR struct usbhost_class_s *(*create)(FAR struct usbhost_hubport_s *hub,
                                        FAR const struct usbhost_id_s *id);

  /* This information uniquely identifies the USB host class implementation that
   * goes with a specific USB device.
   */

  uint8_t nids;                        /* Number of IDs in the id[] array */
  FAR const struct usbhost_id_s *id;   /* An array of ID info. Actual dimension is nids */
};

/* This type represents one endpoint configured by the epalloc() method.
 * The actual form is known only internally to the USB host controller
 * (for example, for an OHCI driver, this would probably be a pointer
 * to an endpoint descriptor).
 */

typedef FAR void *usbhost_ep_t;

/* In the hierarchy of things, there is the host control driver (HCD),
 * represented by struct usbhost_driver_s.  Connected to the HCD are one
 * or more hubs.  At a minimum, the root hub is always present. Each hub
 * has from 1 to 4 ports.

/* Every class connects to the host controller driver (HCD) via a port on a
 * hub.  That hub may be an external hub or the internal, root hub.  The
 * root hub is managed by the HCD.  This structure describes that state of
 * that port and provides the linkage to the parent hub in that event that
 * the port is on an external hub.
 *
 * The root hub port can be distinguish because it has parent == NULL.
 */

struct usbhost_hubport_s
{
  FAR struct usbhost_driver_s *drvr;    /* Common host driver */
#ifdef CONFIG_USBHOST_HUB
  FAR struct usbhost_hubport_s *parent; /* Parent hub (NULL=root hub) */
#endif
  usbhost_ep_t ep0;                     /* Control endpoint, ep0 */
  uint8_t port;                         /* Hub port index */
  uint8_t funcaddr;                     /* Device function address */
  uint8_t speed;                        /* Device speed */
};

/* The root hub port differs in that it includes a data set that is used to
 * manage the generation of unique device function addresses on all
 * downstream ports.
 */

struct usbhost_roothubport_s
{
  /* This structure must appear first so that this structure is cast-
   * compatible with usbhost_hubport_s.
   */

  struct usbhost_hubport_s hport;       /* Common hub port definitions */
  struct usbhost_devaddr_s devgen;      /* Address generation data */
};

/* struct usbhost_class_s provides access from the USB host driver to the
 * USB host class implementation.
 */

struct usbhost_class_s
{
 /* Class instances are associated with devices connected on one port on a
  * hub and are represented by this structure.
  */

  FAR struct usbhost_hubport_s *hport;  /* The port used by this class instance */

  /* Provides the configuration descriptor to the class.  The configuration
   * descriptor contains critical information needed by the class in order to
   * initialize properly (such as endpoint selections).
   */

  int (*connect)(FAR struct usbhost_class_s *devclass,
                 FAR const uint8_t *configdesc,
                 int desclen);

  /* This method informs the class that the USB device has been disconnected. */

  int (*disconnected)(FAR struct usbhost_class_s *devclass);
};

/* This structure describes one endpoint.  It is used as an input to the
 * epalloc() method.  Most of this information comes from the endpoint
 * descriptor.
 */

struct usbhost_epdesc_s
{
  FAR struct usbhost_hubport_s *hport; /* Hub port that supports the endpoint */
  uint8_t addr;                  /* Endpoint address */
  bool in;                       /* Direction: true->IN */
  uint8_t xfrtype;               /* Transfer type.  See USB_EP_ATTR_XFER_* in usb.h */
  uint8_t interval;              /* Polling interval */
  uint16_t mxpacketsize;         /* Max packetsize */
};

/* struct usbhost_connection_s provides as interface between platform-specific
 * connection monitoring and the USB host driver connection and enumeration
 * logic.
 */

struct usbhost_connection_s
{
  /* Wait for a device to connect or disconnect. */

  int (*wait)(FAR struct usbhost_connection_s *conn, FAR const bool *connected);

  /* Enumerate the device connected on a root hub port.  As part of this
   * enumeration process, the driver will (1) get the device's configuration
   * descriptor, (2) extract the class ID info from the configuration
   * descriptor, (3) call usbhost_findclass() to find the class that supports
   * this device, (4) call the create() method on the struct usbhost_registry_s
   * interface to get a class instance, and finally (5) call the connect()
   * method of the struct usbhost_class_s interface.  After that, the class is
   * in charge of the sequence of operations.
   */

  int (*enumerate)(FAR struct usbhost_connection_s *conn, int rhpndx);
};

/* Callback type used with asynchronous transfers */

typedef CODE void (*usbhost_asynch_t)(FAR void *arg, int result);

/* struct usbhost_driver_s provides access to the USB host driver from the
 * USB host class implementation.
 */

struct usbhost_driver_s
{
  /* Configure endpoint 0.  This method is normally used internally by the
   * enumerate() method but is made available at the interface to support
   * an external implementation of the enumeration logic.
   */

  int (*ep0configure)(FAR struct usbhost_driver_s *drvr, usbhost_ep_t ep0,
                      uint8_t funcaddr, uint16_t maxpacketsize);

  /* Allocate and configure an endpoint. */

  int (*epalloc)(FAR struct usbhost_driver_s *drvr,
                 FAR const struct usbhost_epdesc_s *epdesc,
                 FAR usbhost_ep_t *ep);
  int (*epfree)(FAR struct usbhost_driver_s *drvr, FAR usbhost_ep_t ep);

  /* Some hardware supports special memory in which transfer descriptors can
   * be accessed more efficiently.  The following methods provide a mechanism
   * to allocate and free the transfer descriptor memory.  If the underlying
   * hardware does not support such "special" memory, these functions may
   * simply map to kmm_malloc and kmm_free.
   *
   * This interface was optimized under a particular assumption.  It was assumed
   * that the driver maintains a pool of small, pre-allocated buffers for descriptor
   * traffic.  NOTE that size is not an input, but an output:  The size of the
   * pre-allocated buffer is returned.
   */

  int (*alloc)(FAR struct usbhost_driver_s *drvr,
               FAR uint8_t **buffer, FAR size_t *maxlen);
  int (*free)(FAR struct usbhost_driver_s *drvr, FAR uint8_t *buffer);

  /*   Some hardware supports special memory in which larger IO buffers can
   *   be accessed more efficiently.  This method provides a mechanism to allocate
   *   the request/descriptor memory.  If the underlying hardware does not support
   *   such "special" memory, this functions may simply map to kmm_malloc.
   *
   *   This interface differs from DRVR_ALLOC in that the buffers are variable-sized.
   */

  int (*ioalloc)(FAR struct usbhost_driver_s *drvr,
                 FAR uint8_t **buffer, size_t buflen);
  int (*iofree)(FAR struct usbhost_driver_s *drvr, FAR uint8_t *buffer);

  /* Process a IN or OUT request on the control endpoint.  These methods
   * will enqueue the request and wait for it to complete.  Only one transfer may
   * be queued; Neither these methods nor the transfer() method can be called again
   * until the control transfer methods returns.
   *
   * These are blocking methods; these methods will not return until the
   * control transfer has completed.
   */

  int (*ctrlin)(FAR struct usbhost_driver_s *drvr, usbhost_ep_t ep0,
                FAR const struct usb_ctrlreq_s *req,
                FAR uint8_t *buffer);
  int (*ctrlout)(FAR struct usbhost_driver_s *drvr, usbhost_ep_t ep0,
                 FAR const struct usb_ctrlreq_s *req,
                 FAR const uint8_t *buffer);

  /* Process a request to handle a transfer descriptor.  This method will
   * enqueue the transfer request and wait for it to complete.  Only one transfer may
   * be queued; Neither this method nor the ctrlin or ctrlout methods can be called
   * again until this function returns.
   *
   * - 'transfer' is a blocking method; this method will not return until the
   *   transfer has completed.
   * - 'asynch' will return immediately.  When the transfer completes, the
   *    the callback will be invoked with the provided transfer.  This method
   *    is useful for receiving interrupt transfers which may come
   *    infrequently.
   */

  int (*transfer)(FAR struct usbhost_driver_s *drvr, usbhost_ep_t ep,
                  FAR uint8_t *buffer, size_t buflen);
#ifdef CONFIG_USBHOST_ASYNCH
  int (*asynch)(FAR struct usbhost_driver_s *drvr, usbhost_ep_t ep,
                  FAR uint8_t *buffer, size_t buflen,
                  usbhost_asynch_t callback, FAR void *arg);
#endif

  /* Called by the class when an error occurs and driver has been disconnected.
   * The USB host driver should discard the handle to the class instance (it is
   * stale) and not attempt any further interaction with the class driver instance
   * (until a new instance is received from the create() method).
   */

  void (*disconnect)(FAR struct usbhost_driver_s *drvr);
};

/************************************************************************************
 * Public Data
 ************************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/************************************************************************************
 * Public Functions
 ************************************************************************************/

/************************************************************************************
 * Name: usbhost_registerclass
 *
 * Description:
 *   Register a USB host class implementation.  The caller provides an instance of
 *   struct usbhost_registry_s that contains all of the information that will be
 *   needed later to (1) associate the USB host class implementation with a connected
 *   USB device, and (2) to obtain and bind a struct usbhost_class_s instance for
 *   the device.
 *
 * Input Parameters:
 *   devclass - An write-able instance of struct usbhost_registry_s that will be
 *     maintained in a registry.
 *
 * Returned Values:
 *   On success, this function will return zero (OK).  Otherwise, a negated errno
 *   value is returned.
 *
 ************************************************************************************/

int usbhost_registerclass(struct usbhost_registry_s *devclass);

/************************************************************************************
 * Name: usbhost_findclass
 *
 * Description:
 *   Find a USB host class implementation previously registered by
 *   usbhost_registerclass().  On success, an instance of struct usbhost_registry_s
 *   will be returned.  That instance will contain all of the information that will
 *   be needed to obtain and bind a struct usbhost_class_s instance for the device.
 *
 * Input Parameters:
 *   id - Identifies the USB device class that has connect to the USB host.
 *
 * Returned Values:
 *   On success this function will return a non-NULL instance of struct
 *   usbhost_registry_s.  NULL will be returned on failure.  This function can only
 *   fail if (1) id is NULL, or (2) no USB host class is registered that matches the
 *   device class ID.
 *
 ************************************************************************************/

const struct usbhost_registry_s *usbhost_findclass(const struct usbhost_id_s *id);

#ifdef CONFIG_USBHOST_HUB
 /****************************************************************************
 * Name: usbhost_hub_initialize
 *
 * Description:
 *   Initialize the USB hub class.  This function should be called
 *   be platform-specific code in order to initialize and register support
 *   for the USB host storage class.
 *
 * Input Parameters:
 *   None
 *
 * Returned Values:
 *   On success this function will return zero (OK);  A negated errno value
 *   will be returned on failure.
 *
 ****************************************************************************/

int usbhost_hub_initialize(void);
#endif

#ifdef CONFIG_USBHOST_MSC
/****************************************************************************
 * Name: usbhost_storageinit
 *
 * Description:
 *   Initialize the USB host storage class.  This function should be called
 *   be platform-specific code in order to initialize and register support
 *   for the USB host storage class.
 *
 * Input Parameters:
 *   None
 *
 * Returned Values:
 *   On success this function will return zero (OK);  A negated errno value
 *   will be returned on failure.
 *
 ****************************************************************************/

int usbhost_storageinit(void);
#endif

#ifdef CONFIG_USBHOST_HIDKBD
/****************************************************************************
 * Name: usbhost_kbdinit
 *
 * Description:
 *   Initialize the USB storage HID keyboard class driver.  This function
 *   should be called be platform-specific code in order to initialize and
 *   register support for the USB host HID keyboard class device.
 *
 * Input Parameters:
 *   None
 *
 * Returned Values:
 *   On success this function will return zero (OK);  A negated errno value
 *   will be returned on failure.
 *
 ****************************************************************************/

int usbhost_kbdinit(void);
#endif

#ifdef CONFIG_USBHOST_HIDMOUSE
/****************************************************************************
 * Name: usbhost_mouse_init
 *
 * Description:
 *   Initialize the USB storage HID mouse class driver.  This function
 *   should be called be platform-specific code in order to initialize and
 *   register support for the USB host HID mouse class device.
 *
 * Input Parameters:
 *   None
 *
 * Returned Values:
 *   On success this function will return zero (OK);  A negated errno value
 *   will be returned on failure.
 *
 ****************************************************************************/

int usbhost_mouse_init(void);
#endif

/****************************************************************************
 * Name: usbhost_wlaninit
 *
 * Description:
 *   Initialize the USB WLAN class driver.  This function should be called
 *   be platform-specific code in order to initialize and register support
 *   for the USB host class device.
 *
 * Input Parameters:
 *   None
 *
 * Returned Values:
 *   On success this function will return zero (OK);  A negated errno value
 *   will be returned on failure.
 *
 ****************************************************************************/

int usbhost_wlaninit(void);

/*******************************************************************************
 * Name: usbhost_enumerate
 *
 * Description:
 *   This is a share-able implementation of most of the logic required by the
 *   driver enumerate() method.  This logic within this method should be common
 *   to all USB host drivers.
 *
 *   Enumerate the connected device.  As part of this enumeration process,
 *   the driver will (1) get the device's configuration descriptor, (2)
 *   extract the class ID info from the configuration descriptor, (3) call
 *   usbhost_findclass() to find the class that supports this device, (4)
 *   call the create() method on the struct usbhost_registry_s interface
 *   to get a class instance, and finally (5) call the configdesc() method
 *   of the struct usbhost_class_s interface.  After that, the class is in
 *   charge of the sequence of operations.
 *
 * Input Parameters:
 *   hub - The hub that manages the new class.
 *   devclass - If the class driver for the device is successful located
 *      and bound to the hub, the allocated class instance is returned into
 *      this caller-provided memory location.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   - Only a single class bound to a single device is supported.
 *   - Called from a single thread so no mutual exclusion is required.
 *   - Never called from an interrupt handler.
 *
 *******************************************************************************/

int usbhost_enumerate(FAR struct usbhost_hubport_s *hub,
                      FAR struct usbhost_class_s **devclass);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __INCLUDE_NUTTX_USB_USBHOST_H */

