# Installing TextOS/2 to Disk


## Create a disk image with DD
```
dd if=/dev/zero of=disk.img bs=1G count=1
```
This will create a 1GB disk image named `disk.img`. To increase the size, change the `count` argument

## Create the partition and partition table

Create a new partition with FDISK and set the partition type to "A7"
Make sure to leave enough room between the MBR and the first partition for the kernel, initrd and the boot loader (16384 (8MB) Sectors should be fine).

## Create a GRUB core image

```
grub-mkimage -O i386-pc -o initrd/install/CORE.IMG -p '' configfile multiboot vbe
```

## Calculate the Kernel offset

Take the size of the bootloader (core.img), divide that by 512, and add 2. This is where TextOS will store the kernel.

## Modify the grub configuration file

edit the file at misc/grub.cfg and replace the KERNEL_OFFSET with the calculated offset.

## Recreate the GRUB core image with embedded configuration file

```
grub-mkimage -O i386-pc -o initrd/install/CORE.IMG -p '' -c "PATH/TO/CONFIG" configfile multiboot vbe
```

## Copy kernel.bin to the initrd

```
cp kernel.bin initrd/install/KERNEL.BIN
```

## Installing to disk

Boot TextOS with disk attached. As TextOS is booting, all detected XFS partitions will be displayed.

Example:
```
Found XFS partition at Disk0/1!
```
In this case, Disk0 is the disk identifier, and /1 is the partition number.

To partition the XFS partition run,
```
xfs partition Disk0
```

Then, to copy the kernel and boot loader to disk..
```
xfs install Disk0 /PATH/TO/CORE.IMG /PATH/TO/KERNEL.BIN
```
