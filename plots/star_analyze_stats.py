# print the configurations of trials that result in all bursty flows not slowed down when sharing with continuous flows
def trials_all_zero_fct_slowdown(dir, starttime_list, initialwindow_list, numcontinous, numbursty):
	for iw in initialwindow_list:
		for start in starttime_list:
			fctslowdown_allseedslist = list()
			for seed in range(1,11):
				# Read fct from file
				cwbfile = dir+"star-burst-tolerance-flow-"+str(iw)+"-"+str(start)+"-0-"+str(seed)+".data"
				bonlyfile = dir+"star-burst-tolerance-flow-"+str(iw)+"-"+str(start)+"-1-"+str(seed)+".data"
				cwb_fct = dict()
				bonly_fct = dict()
				with open(cwbfile) as cwbf:
					count = 0
					for line in cwbf:
						count += 1
						if count <= 1: continue
						array = [x for x in line.split()]
						flowsize = float(array[1])
						fct = float(array[2])
						cwb_fct[flowsize] = fct
				with open(bonlyfile) as bonlyf:
					count = 0
					for line in bonlyf:
						count += 1
						if count <= numcontinous+1: continue
						array = [x for x in line.split()]
						flowsize = float(array[1])
						fct = float(array[2])
						bonly_fct[flowsize] = fct
				
				# Calculate fct slowdown
				fctslowdown = list()
				#assert(len(cwb_fct) == numbursty)
				#assert(len(bonly_fct) == numbursty)
				for cwb_flow_size, cwb_flow_fct in cwb_fct.items():
					for bonly_flow_size, bonly_flow_fct in bonly_fct.items():
						if cwb_flow_size == bonly_flow_size:
							fctslowdown.append(cwb_flow_fct/bonly_flow_fct)
							break
				fctslowdown_allseedslist.append(fctslowdown)
				#print(str(iw)+" "+str(start))
				#print(fctslowdown)
				#print(cwb_fct)
				#print(bonly_fct)

			# Inspect which trials have all flows with 0 FCT slowdown
			for index,slowdownlist in enumerate(fctslowdown_allseedslist):
				should_print = True
				for sd in slowdownlist:
					if sd != 1:
						should_print = False
						break
				if should_print:
					print(f"iw={iw},start={start},seed={index}")

	return


def average_throughput(dir, b_list, b_factor, numqueuesperport, numnodes, numsinks):
	outfile_tp = dir + "average_throughput.txt"
	outfile_tp2 = dir + "average_throughput_after2.txt"
	open(outfile_tp, 'w').close()
	open(outfile_tp2, 'w').close()
	
	# we look at the sink port for the continuous flows now instead of the bursty flow, thus the extra -4
	target_queue_index = numqueuesperport*(numnodes+numsinks)-1-4
	for b in b_list:
		buffer = b*b_factor
		throughput_list = list()
		# Read
		torfile = dir+"tor-single-1-101-"+str(buffer)+".stat"
		with open(torfile) as torf:
			line_count = 0
			for line in torf:
				line_count += 1
				if line_count <= 1: continue
				array = [x for x in line.split()]
				sentbytes = float(array[3+5*target_queue_index+2])
				throughput_list.append(sentbytes)

		# Write
		print(len(throughput_list))
		f_tp = open(outfile_tp,"a")
		f_tp.write(str(buffer)+"\t"+str(sum(throughput_list)/len(throughput_list))+"\n")
		f_tp.close()

		f_tp2 = open(outfile_tp2,"a")
		f_tp2.write(str(buffer)+"\t"+str(sum(throughput_list[200:])/len(throughput_list[200:]))+"\n")
		f_tp2.close()
	
	return


if __name__ == "__main__":
	dir = "/u/az6922/data/"
	start_list = [0,4500]
	iw_list = [5,25,50,75,100]
	numcontinuous = 10
	numbursty = 100
	# trials_all_zero_fct_slowdown(dir, start_list, iw_list, numcontinuous, numbursty)
	b_list = list(range(5,10))
	b_factor = 1000000
	numqueuesperport = 3
	numnodes = 20
	numsinks = 2
	average_throughput(dir,b_list,b_factor,numqueuesperport,numnodes,numsinks)
