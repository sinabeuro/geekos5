vboxmanage storageattach geekOS --storagectl "IDE" --port 0 --device 0 --medium none

vboxmanage storageattach geekOS --storagectl "IDE" --port 0 --device 1 --medium none

VBoxManage closemedium disk C:\Users\Administrator\Desktop\share\project5\build\diskc.vdi

VBoxManage closemedium disk C:\Users\Administrator\Desktop\share\project5\build\diskd.vdi

vboxmanage storageattach geekOS --storagectl "IDE" --port 0 --device 0 --type hdd --medium C:\Users\Administrator\Desktop\share\project5\build\diskc.vdi

vboxmanage storageattach geekOS --storagectl "IDE" --port 0 --device 1 --type hdd --medium C:\Users\Administrator\Desktop\share\project5\build\diskd.vdi
