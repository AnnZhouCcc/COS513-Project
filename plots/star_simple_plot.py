import pandas as pd
import matplotlib.pyplot as plt

dir = "/u/az6922/data/"
file = dir + "tor-single-2-101-0.stat"

#df = pd.read_csv(file, delim_whitespace=True)


def plot_port10():
	qsize_list = list()
	buffer_list = list()
	time_list = list()
	with open(file) as f:
		line_count = -1
		for line in f:
			line_count += 1
			if line_count <= 0: continue
			array = [x for x in line.split()]
			buffer_list.append(float(array[2])*float(array[1])*10000)
			time_list.append(int(array[0])-2000000000)
			qsize_list.append(int(array[53]))

	#plt.plot(time_list[:1000000],buffer_list[:1000000],label="occupied buffer")
	plt.plot(time_list,buffer_list,label="occupied buffer")
	#plt.plot(time_list[:1000000],qsize_list[:1000000],label="port0 qsize")
	plt.plot(time_list,qsize_list,label="port10 qsize")
	plt.title("port10 qsize over time")
	plt.legend()
	plt.savefig(dir+"star_simple_plot_port10.pdf")


def plot_all(numqueues):
	qsize_list_arr = []
	for i in range(numqueues):
		qsize_list_arr.append(list())
	buffer_list = list()
	time_list = list()
	with open(file) as f:
		line_count = -1
		for line in f:
			line_count += 1
			if line_count <= 0: continue
			array = [x for x in line.split()]
			buffer_list.append(float(array[2])*float(array[1])*10000)
			time_list.append(int(array[0])-2000000000)
			for i in range(numqueues):
				qsize_list_arr[i].append(int(array[3+5*i]))

	#plt.plot(time_list[:1000000],buffer_list[:1000000],label="occupied buffer")
	plt.plot(time_list,buffer_list,label="occupied buffer")
	#plt.plot(time_list[:1000000],qsize_list[:1000000],label="port0 qsize")
	for i in range(numqueues):
		plt.plot(time_list,qsize_list_arr[i],label="port"+str(i)+" qsize")
	plt.title("buffer & qsize of all ports over time")
	plt.legend()
	plt.savefig(dir+"star_simple_plot_all.pdf")


def plot_buffer():
	buffer_list = list()
	time_list = list()
	with open(file) as f:
		line_count = -1
		for line in f:
			line_count += 1
			if line_count<=0: continue
			array = [x for x in line.split()]
			buffer_list.append(float(array[2])*float(array[1])*10000)
			time_list.append(int(array[0])-2000000000)

	plt.plot(time_list,buffer_list,label="occupied buffer")
	plt.title("occupied buffer over time")
	plt.legend()
	plt.savefig(dir+"star_simple_plot_buffer.pdf")

# sentbytes: plot_generic(10, 2, "sentbytes")
# droppedbytes: plot_generic(10,3,"droppedbytes")
def plot_generic(numqueues, offset, name):
	qsize_list_arr = []
	for i in range(numqueues):
		qsize_list_arr.append(list())
	buffer_list = list()
	time_list = list()
	with open(file) as f:
		line_count = -1
		for line in f:
			line_count += 1
			if line_count <= 0: continue
			array = [x for x in line.split()]
			buffer_list.append(float(array[2])*float(array[1])*10000)
			time_list.append(int(array[0])-2000000000)
			for i in range(numqueues):
				qsize_list_arr[i].append(int(array[3+5*i+offset]))

	#plt.plot(time_list,buffer_list,label="occupied buffer")
	for i in range(numqueues):
		plt.plot(time_list,qsize_list_arr[i],label="port"+str(i)+" "+name)
	plt.title(name+" of all ports over time")
	plt.legend()
	plt.savefig(dir+"star_simple_plot_"+name+".pdf")


if __name__ == "__main__":
	plot_generic(10,3,"droppedbytes")
