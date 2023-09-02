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


def average_throughput(dir, b_list, b_factor, numqueuesperport, numnodes, numsinks, time_after):
	outfile_tp = dir + "average_throughput.txt"
	outfile_tp2 = dir + "average_throughput_after"+str(time_after)+".txt"
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
		f_tp2.write(str(buffer)+"\t"+str(sum(throughput_list[time_after:])/len(throughput_list[time_after:]))+"\n")
		f_tp2.close()
	
	return


def total_drop(file, numqueuesperport, numnodes, numsinks):	
	target_queue_index_sink0 = numqueuesperport*(numnodes+numsinks)-5
	target_queue_index_sink1 = numqueuesperport*(numnodes+numsinks)-1
	sum_numdrop_sink0 = 0
	sum_numdrop_sink1 = 0
	sum_unittime_sink0 = 0
	sum_unittime_sink1 = 0

	# Read buffer from file
	with open(file) as f:
		line_count = 0
		for line in f:
			line_count += 1
			if line_count <= 1: continue
			array = [x for x in line.split()]
			drop_sink0 = float(array[3+5*target_queue_index_sink0+3])
			drop_sink1 = float(array[3+5*target_queue_index_sink1+3])
			sum_numdrop_sink0 += drop_sink0
			sum_numdrop_sink1 += drop_sink1
			if drop_sink0 != 0:
				sum_unittime_sink0 += 1
			if drop_sink1 != 0:
				sum_unittime_sink1 += 1

	# Write out stats
	print(f'number of drops: sink0={sum_numdrop_sink0}, sink1={sum_numdrop_sink1}')
	print(f'duration of drops: sink0={sum_unittime_sink0}, sink1={sum_unittime_sink1}')
		
	return


def write_fct_comparison_hetero_rtt_bb():
	bothfile = dir+"fcts-hetero-rtt-1-101-3.fct"
	longfile = dir+"fcts-hetero-rtt-1-101-7.fct"
	shortfile = dir+"fcts-hetero-rtt-1-101-8.fct"
	writefile = dir+"fcts_378.txt"

	mapsizefcts = dict()
	with open(bothfile) as bothf:
		count = 0
		for line in bothf:
			count += 1
			if count <= 1: continue
			array = [x for x in line.split()]
			flowsize = float(array[1])
			fct = float(array[2])
			priority = int(array[5])
			if flowsize not in mapsizefcts:
				mapsizefcts[flowsize] = dict()
			if priority == 1:
				label = "long_both"
				if label in mapsizefcts[flowsize]:
					print(f"Error! long_both already exists, bothfile, flowsize={flowsize}, fct={fct}, priority={priority}")
				mapsizefcts[flowsize][label] = fct
			elif priority == 2:
				label = "short_both"
				if label in mapsizefcts[flowsize]:
					print(f"Error! short_both already exists, bothfile, flowsize={flowsize}, fct={fct}, priority={priority}")
				mapsizefcts[flowsize][label] = fct

	with open(longfile) as longf:
		count = 0
		for line in longf:
			count += 1
			if count <= 1: continue
			array = [x for x in line.split()]
			flowsize = float(array[1])
			fct = float(array[2])
			priority = int(array[5])
			if priority == 2:
				continue
			if flowsize not in mapsizefcts:
				print(f"Error! entry not already exist, longfile, flowsize={flowsize}, fct={fct}, priority={priority}")
			label = "long_only"
			if label in mapsizefcts[flowsize]:
				print(f"Error! long_only already exists, longfile, flowsize={flowsize}, fct={fct}, priority={priority}")
			mapsizefcts[flowsize][label] = fct

	with open(shortfile) as shortf:
		count = 0
		for line in shortf:
			count += 1
			if count <= 1: continue
			array = [x for x in line.split()]
			flowsize = float(array[1])
			fct = float(array[2])
			priority = int(array[5])
			if priority == 1:
				continue
			if flowsize not in mapsizefcts:
				print(f"Error! entry not already exist, shortfile, flowsize={flowsize}, fct={fct}, priority={priority}")
			label = "short_only"
			if label in mapsizefcts[flowsize]:
				print(f"Error! short_only already exists, shortfile, flowsize={flowsize}, fct={fct}, priority={priority}")
			mapsizefcts[flowsize][label] = fct

	with open(writefile, "w") as f:
		f.write(f"flowsize\tlong_both\tlong_only\tlong_ratio\tshort_both\tshort_only\tshort_ratio\n")
		for flowsize,fcts in mapsizefcts.items():
			long_both_fct = fcts["long_both"]
			short_both_fct = fcts["short_both"]
			long_only_fct = fcts["long_only"]
			short_only_fct = fcts["short_only"]
			f.write(f"{flowsize}\t{long_both_fct}\t{long_only_fct}\t{long_both_fct/long_only_fct}\t{short_both_fct}\t{short_only_fct}\t{short_both_fct/short_only_fct}\n")

	return


if __name__ == "__main__":
	dir = "/u/az6922/data/"
	start_list = [0,4500]
	iw_list = [5,25,50,75,100]
	numcontinuous = 10
	numbursty = 100
	# trials_all_zero_fct_slowdown(dir, start_list, iw_list, numcontinuous, numbursty)
	b_list = list(range(30,41))
	b_factor = 100000
	numqueuesperport = 3
	numnodes = 20
	numsinks = 2
	time_after = 300
	file = dir+"hetero-rtt-cc-after-aug27/tor-hetero-rtt-1-101-3.stat"
	#average_throughput(dir,b_list,b_factor,numqueuesperport,numnodes,numsinks, time_after)
	#total_drop(file, numqueuesperport, numnodes, numsinks)
	write_fct_comparison_hetero_rtt_bb()
