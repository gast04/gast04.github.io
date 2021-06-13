import os
import idaapi

dir_path = "<path>"

# section to dump start and end address
start_addr = 0x7364b1e8ec
end_addr =   0x7364b1e8fc
file_name = os.path.join(dir_path, "text_section")

# LOAD section
start_addr = 0x7364b1e8fc
end_addr =   0x736899fb10
file_name = os.path.join(dir_path, "LOAD_section1")

print("Dumping {} bytes".format(end_addr - start_addr))

f = open(file_name, "wb")
for i in range(int((end_addr - start_addr)/4)):
  b = idaapi.get_bytes(start_addr + i*4, 4)
  f.write(b)

f.close()
print("dumped to {}".format(file_name))
