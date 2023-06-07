import pandas as pd
import matplotlib.pyplot as plt

tcpname = "timely"
version = 7
dir = "/u/az6922/data/"
file = dir + "tor-simple-noincast-continuous-"+tcpname+"-"+str(version)+".stat"

#df = pd.read_csv(file, delim_whitespace=True)

def plot():
	buffer_list = list()
	sentbytes_list = list()
	time_list = list()
	with open(file) as f:
		line_count = -1
		for line in f:
			line_count += 1
			if line_count%2 == 0: continue
			array = [x for x in line.split()]
			buffer_list.append(float(array[3])*float(array[2])*1000)
			time_list.append(int(array[0])-2000000000)
			assert(int(array[1])==0)
			sum_sentbytes = 0
			for port in range(0,40):
				sum_sentbytes += int(array[4+port*5+2])
			sentbytes_list.append(sum_sentbytes)

	plt.plot(time_list,sentbytes_list,label="total sentbytes")
	plt.plot(time_list,buffer_list,label="occupied buffer")
	plt.title(tcpname+", leaf0")
	plt.legend()
	plt.savefig(dir+"simple_inspect_plot.pdf")


def plot_port0():
	qsize_list = list()
	buffer_list = list()
	time_list = list()
	with open(file) as f:
		line_count = -1
		for line in f:
			line_count += 1
			if line_count%2 == 0: continue
			array = [x for x in line.split()]
			buffer_list.append(float(array[3])*float(array[2])*10000)
			time_list.append(int(array[0])-2000000000)
			qsize_list.append(int(array[4]))

	#plt.plot(time_list[:1000000],buffer_list[:1000000],label="occupied buffer")
	plt.plot(time_list,buffer_list,label="occupied buffer")
	#plt.plot(time_list[:1000000],qsize_list[:1000000],label="port0 qsize")
	plt.plot(time_list,qsize_list,label="port0 qsize")
	plt.title(tcpname+", leaf0")
	plt.legend()
	plt.savefig(dir+"simple_inspect_plot_port0.pdf")


def plot_buffer():
	buffer_list = list()
	time_list = list()
	with open(file) as f:
		line_count = -1
		for line in f:
			line_count += 1
			if line_count%2 == 0: continue
			array = [x for x in line.split()]
			buffer_list.append(float(array[3])*float(array[2])*10000)
			time_list.append(int(array[0])-2000000000)

	plt.plot(time_list,buffer_list,label="occupied buffer")
	plt.title(tcpname+", leaf0")
	plt.legend()
	plt.savefig(dir+"simple_inspect_plot_buffer.pdf")


def inspect():
	with open(file) as f:
		line_count = -1
		for line in f:
			line_count += 1
			# if line_count == 0: continue
			if line_count%2 == 0: continue
			array = [x for x in line.split()]
			have_non_zero = False
			for port in range(0,40):
				qsize = int(array[4+port*5])
				sentbytes = int(array[4+port*5+2])
				#if qsize>0 or sentbytes>0:
				if qsize>0:
					if not have_non_zero:
						print(f"leaf{array[1]}, timestamp={array[0]}, buffer={array[3]}")
						have_non_zero = True
					print(f"port={port}, qsize={qsize}, sentbytes={sentbytes}")


queuesize = dict()
def count():
	with open(file) as f:
		line_count = -1
		for line in f:
			line_count += 1
			if line_count%2 == 0: continue
			array = [x for x in line.split()]
			for port in range(0,40):
				qsize = int(array[4+port*5])
				if qsize > 0:
					if port in queuesize:
						queuesize[port] += qsize
					else:
						queuesize[port] = qsize
	print(queuesize)


if __name__ == "__main__":
	plot_buffer()
