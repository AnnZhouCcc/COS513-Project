import pandas as pd
import matplotlib.pyplot as plt


#df = pd.read_csv(file, delim_whitespace=True)


def plot_port10(file):
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


def plot_all(file,plotname,numqueues):
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


def plot_all_range(file,numqueues,plotstart,plotend):
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


def plot_sink_range(file,plotname,numsenders,numsinks,numqueuespport,plotstart,plotend,timeoffsetns):
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
			time_list.append(int(array[0])-timeoffsetns)
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


def plot_selected(file,numservers,numsinks):
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


def plot_buffer(file):
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
def plot_generic(file,numqueues, offset, name):
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
def plot_separate(file,plotname,numqueues, offset, name):
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
def plot_sink_separate(file,plotname,numqueues, queuestart, queueend, offset, name, plotstart, plotend,timeoffsetns):
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
			time_list.append(int(array[0])-timeoffsetns)
			for i in range(queuestart,queueend):
				qsize_list_arr[i].append(float(array[3+5*i+offset]))

	#print(name)
	#print(qsize_list_arr[4])
	plt.clf()
	fig, axs = plt.subplots(queueend-queuestart, figsize=(15,6*(queueend-queuestart)))
	fig.suptitle(name+" of all ports over time")
	for i in range(queuestart,queueend):
		axs[i-queuestart].plot(time_list[plotstart:plotend],qsize_list_arr[i][plotstart:plotend])
		plt.title("queue index "+str(i)+" "+name)
	plt.savefig(dir+"star_"+plotname+"_plot_sink_separate_"+name+"_"+str(plotstart)+"_"+str(plotend)+".pdf")


if __name__ == "__main__":
	dir = "/u/az6922/data/"
	v = 3
	cc = 1
	file = dir + "tor-bbr-"+str(cc)+"-101-"+str(v)+".stat"
	plotname = "bbr-v"+str(v)
	numcontinuous = 1
	numbursty = 0
	numnodes = numcontinuous+numbursty
	numsinks = 1
	numqueuesperport = 3
	timestart = 0
	timeend = 2400 #1600 #10000
	timeoffsetns = 1000000000
	plot_sink_range(file,plotname,numnodes,numsinks,numqueuesperport,timestart,timeend,timeoffsetns)
	plot_sink_separate(file,plotname,numqueuesperport*(numnodes+numsinks),numqueuesperport*numnodes,numqueuesperport*(numnodes+numsinks),numsinks,"sentbytes",timestart,timeend,timeoffsetns)
	plot_sink_separate(file,plotname,numqueuesperport*(numnodes+numsinks),numqueuesperport*numnodes,numqueuesperport*(numnodes+numsinks),3,"droppedbytes",timestart,timeend,timeoffsetns)
	"""
	for b in range(10,26):
		buffer = int(b*100000)
		file = dir + "tor-hetero-rtt-1-101-"+str(buffer)+".stat"
		plotname = "hetero-rtt-cc-long-b"+str(buffer)
		print("buffer="+str(buffer)+", reading "+file)
		numcontinuous = 10
		numbursty = 10
		numnodes = numcontinuous+numbursty
		numsinks = 2
		numqueuesperport = 3
		timestart = 0
		timeend = 1500
		timeoffsetns = 1000000000
		plot_sink_range(file,plotname,numnodes,numsinks,numqueuesperport,timestart,timeend,timeoffsetns)
		plot_sink_separate(file,plotname,numqueuesperport*(numnodes+numsinks),numqueuesperport*numnodes,numqueuesperport*(numnodes+numsinks),numsinks,"sentbytes",timestart,timeend,timeoffsetns)
		plot_sink_separate(file,plotname,numqueuesperport*(numnodes+numsinks),numqueuesperport*numnodes,numqueuesperport*(numnodes+numsinks),3,"droppedbytes",timestart,timeend,timeoffsetns)
	"""
	#plot_generic(4,3,"droppedbytes")
	#plot_generic(22,2,"sentbytes")
	#plot_generic(4,1,"throughput")
	#plot_all(22)
	#plot_all_range(4,5250000,5350000)
	#plot_separate(22,2,"sentbytes")
	#plot_separate(22,3,"droppedbytes")
