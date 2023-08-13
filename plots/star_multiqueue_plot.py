import pandas as pd
import matplotlib.pyplot as plt

dir = "/u/az6922/data/"
file = dir + "tor-single-1-101-5.stat"
plotname = "rbt-rerun-v5"

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

	plt.figure().set_figwidth(15)
	plt.plot(time_list,buffer_list,label="occupied buffer")
	for i in range(numqueues):
		plt.plot(time_list,qsize_list_arr[i],label="port"+str(i)+" qsize")
	plt.title("buffer & qsize of all ports over time")
	plt.legend()
	plt.savefig(dir+"star_"+plotname+"_plot_all.pdf")


def plot_all_range(numqueues,plotstart,plotend):
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

	plt.clf()
	plt.figure().set_figwidth(15)
	plt.plot(time_list[plotstart:plotend],buffer_list[plotstart:plotend],label="occupied buffer")
	for i in range(numqueues):
		plt.plot(time_list[plotstart:plotend],qsize_list_arr[i][plotstart:plotend],label="port"+str(i)+" qsize")
	plt.title("buffer & qsize of all ports over time ["+str(plotstart)+","+str(plotend)+"]")
	plt.legend()
	plt.savefig(dir+"star_simple_plot_all_"+str(plotstart)+"_"+str(plotend)+".pdf")


def plot_sink_range(numsenders,numsinks,numqueuespport,plotstart,plotend):
	queue_start = numsenders * numqueuespport
	queue_end = (numsenders + numsinks) * numqueuespport
	numqueues = (numsenders + numsinks) * numqueuespport
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
			for i in range(queue_start,queue_end):
				qsize_list_arr[i].append(int(array[3+5*i]))

	plt.clf()
	plt.figure().set_figwidth(15)
	plt.plot(time_list[plotstart:plotend],buffer_list[plotstart:plotend],label="occupied buffer")
	for i in range(queue_start, queue_end):
		port_index = i/5
		queue_index = i%5
		plt.plot(time_list[plotstart:plotend],qsize_list_arr[i][plotstart:plotend],label="port"+str(port_index)+" queue "+str(queue_index))
	plt.title("buffer & qsize of sink ports over time ["+str(plotstart)+","+str(plotend)+"]")
	plt.legend()
	plt.savefig(dir+"star_"+plotname+"_plot_sink_"+str(plotstart)+"_"+str(plotend)+".pdf")


def plot_selected(numservers,numsinks):
	numqueues = numservers+numsinks
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
	for i in range(numservers,numqueues):
		plt.plot(time_list,qsize_list_arr[i],label="port"+str(i)+" qsize")
	plt.title("buffer & sink qsize over time")
	plt.legend()
	plt.savefig(dir+"star_simple_plot_selected.pdf")


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
				qsize_list_arr[i].append(float(array[3+5*i+offset]))

	#plt.plot(time_list,buffer_list,label="occupied buffer")
	#for i in range(3,4):
	#	plt.plot(time_list[1572790:1572795],qsize_list_arr[i][1572790:1572795],label="port"+str(i)+" "+name,marker=".")
	for i in range(numqueues):
		plt.plot(time_list,qsize_list_arr[i],label="port"+str(i)+" "+name)
	plt.title(name+" of all ports over time")
	plt.legend()
	plt.savefig(dir+"star_simple_plot_"+name+".pdf")


# sentbytes: plot_generic(10, 2, "sentbytes")
# droppedbytes: plot_generic(10,3,"droppedbytes")
def plot_separate(numqueues, offset, name):
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
				qsize_list_arr[i].append(float(array[3+5*i+offset]))

	fig, axs = plt.subplots(numqueues, figsize=(15,60))
	fig.suptitle(name+" of all ports over time")
	for i in range(numqueues):
		axs[i].plot(time_list,qsize_list_arr[i])
		plt.title("port "+str(i)+" "+name)
	plt.savefig(dir+"star_"+plotname+"_plot_separate_"+name+".pdf")


# sentbytes: plot_generic(10, 2, "sentbytes")
# droppedbytes: plot_generic(10,3,"droppedbytes")
def plot_sink_separate(numqueues, queuestart, queueend, offset, name, plotstart, plotend):
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
			for i in range(queuestart,queueend):
				qsize_list_arr[i].append(float(array[3+5*i+offset]))

	print(len(time_list))
	plt.clf()
	fig, axs = plt.subplots(queueend-queuestart, figsize=(15,6*(queueend-queuestart)))
	fig.suptitle(name+" of all ports over time")
	for i in range(queuestart,queueend):
		axs[i-queuestart].plot(time_list[plotstart:plotend],qsize_list_arr[i][plotstart:plotend])
		plt.title("queue index "+str(i)+" "+name)
	plt.savefig(dir+"star_"+plotname+"_plot_sink_separate_"+name+"_"+str(plotstart)+"_"+str(plotend)+".pdf")


if __name__ == "__main__":
	plot_sink_range(11,2,3,0,1000)
	plot_sink_separate(39,33,39,2,"sentbytes",0,1000)
	plot_sink_separate(39,33,39,3,"droppedbytes",0,1000)

	#plot_generic(4,3,"droppedbytes")
	#plot_generic(22,2,"sentbytes")
	#plot_generic(4,1,"throughput")
	#plot_all(22)
	#plot_all_range(4,5250000,5350000)
	#plot_separate(22,2,"sentbytes")
	#plot_separate(22,3,"droppedbytes")
