def summarize_flow(dir, starttime_list, initialwindow_list, numcontinous, numbursty, s3mode="all"):
	outfile1 = dir + "flow_average_all.txt"
	# outfile2 = dir + "average_slow.txt"
	outfile3 = dir + "flow_num_slow.txt"
	open(outfile1, 'w').close()
	# open(outfile2, 'w').close()
	open(outfile3, 'w').close()
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
				# AnnC: some flows seem to be unlogged
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

			# Write out tables
			# 1. Average FCT slowdown across all flows
			sum_fct_all = 0
			count_fct_all = 0
			for slowdownlist in fctslowdown_allseedslist:
				avg_sd = sum(slowdownlist) / len(slowdownlist)
				sum_fct_all += avg_sd
				count_fct_all += 1
			f1 = open(outfile1, "a")
			f1.write(str(sum_fct_all/count_fct_all)+"\t")
			f1.close()

			# 2. Average FCT slowdown across flows that have been slowed down
			# sum_fct_slow = 0
			# count_fct_slow = 0
			# for slowdownlist in fctslowdown_allseedslist:
			# 	sum_sd = 0
			# 	count_sd = 0
			# 	for sd in slowdownlist:
			# 		if sd > 1:
			# 			sum_sd += sd
			# 			count_sd += 1
			# 	if count_sd > 0:
			# 		avg_sd = sum_sd / count_sd
			# 	else:
			# 		avg_sd = 0
			# 	if avg_sd > 0:
			# 		sum_fct_slow += avg_sd
			# 		count_fct_slow += 1
			# f2 = open(outfile2, "a")
			# f2.write(str(sum_fct_slow/count_fct_slow)+"\t")
			# f2.close()

			# 3. Number of flows that have been slowed down
			count_num_slow = 0
			num_num_slow = 0
			if s3mode == "positive":
				for slowdownlist in fctslowdown_allseedslist:
					count_sd = 0
					for sd in slowdownlist:
						if sd > 1:
							count_sd += 1
					if count_sd > 0:
						count_num_slow += count_sd
						num_num_slow += len(slowdownlist)
				f3 = open(outfile3, "a")
				f3.write(str(count_num_slow/float(num_num_slow))+"\t")
			elif s3mode == "all":
				for slowdownlist in fctslowdown_allseedslist:
					count_sd = 0
					for sd in slowdownlist:
						if sd > 1:
							count_sd += 1
						num_num_slow += 1
					count_num_slow += count_sd
				f3 = open(outfile3, "a")
				f3.write(str(count_num_slow/float(num_num_slow))+"\t")
			f3.close()

		f1 = open(outfile1, "a")
		f1.write("\n")
		f1.close()
		# f2 = open(outfile2, "a")
		# f2.write("\n")
		# f2.close()
		f3 = open(outfile3, "a")
		f3.write("\n")
		f3.close()

	return


# will only read the bursty-only files
def characterize_burst(dir, starttime_list, initialwindow_list, numcontinous, numbursty, numqueuesperport, numnodes, numsinks, pullingns):
	outfile1 = dir + "burst_size1.txt"
	outfile2 = dir + "burst_size2.txt"
	outfile3 = dir + "burst_rate1.txt"
	outfile4 = dir + "burst_rate2.txt"
	open(outfile1, 'w').close()
	open(outfile2, 'w').close()
	open(outfile3, 'w').close()
	open(outfile4, 'w').close()

	for iw in initialwindow_list:
		for start in starttime_list:
			f1 = open(outfile1, "a")
			f1.write(str(iw) + " (" + str(start) + "):\t")
			f1.close()
			f2 = open(outfile2, "a")
			f2.write(str(iw) + " (" + str(start) + "):\t")
			f2.close()
			f3 = open(outfile3, "a")
			f3.write(str(iw) + " (" + str(start) + "):\t")
			f3.close()
			f4 = open(outfile4, "a")
			f4.write(str(iw) + " (" + str(start) + "):\t")
			f4.close()
			for seed in range(1,11):
				# Read from file
				flowfile = dir+"star-burst-tolerance-flow-"+str(iw)+"-"+str(start)+"-1-"+str(seed)+".data"
				bufferfile = dir+"star-burst-tolerance-buffer-"+str(iw)+"-"+str(start)+"-1-"+str(seed)+".data"
				paramfile = dir+"star-burst-tolerance-parameters-"+str(iw)+"-"+str(start)+"-1-"+str(seed)+".data"
				
				# burst_size1
				sum_flowsize = 0
				with open(paramfile) as paramf:
					count = 0
					for line in paramf:
						count += 1
						if count <= numcontinous+1: continue
						array = [x for x in line.split(',')]
						flowsizephrase = array[1]
						phrasearray = [x for x in flowsizephrase.split('=')]
						flowsize = float(phrasearray[1])
						# print(flowsize)
						sum_flowsize += flowsize
				# print(sum_flowsize)	
				f1 = open(outfile1, "a")
				f1.write(str(sum_flowsize)+"\t")
				f1.close()

				# burst_size2
				target_queue_index = numqueuesperport*(numnodes+numsinks)-1
				sum_sentbytes = 0
				with open(bufferfile) as bufferf:
					line_count = 0
					for line in bufferf:
						line_count += 1
						if line_count <= 1: continue
						array = [x for x in line.split()]
						sentbytes = float(array[3+5*target_queue_index+2])
						sum_sentbytes += sentbytes
				f2 = open(outfile2, "a")
				f2.write(str(sum_sentbytes)+"\t")
				f2.close()

				# burst_rate1
				starttime_rate1 = 0
				endtime_rate1 = 0
				highest_sentbytes = 0
				rate1 = 0
				with open(bufferfile) as bufferf:
					line_count = 0
					for line in bufferf:
						line_count += 1
						if line_count <= 1: continue
						array = [x for x in line.split()]
						time = int(array[0])
						sentbytes = float(array[3+5*target_queue_index+2])
						if starttime_rate1 == 0 and sentbytes > 0:
							starttime_rate1 = time
						if sentbytes > highest_sentbytes:
							highest_sentbytes = sentbytes
							endtime_rate1 = time
				if endtime_rate1 == starttime_rate1 and starttime_rate1 > 0:
					rate1 = highest_sentbytes/float(pullingns)
				else:
					rate1 = highest_sentbytes/(endtime_rate1-starttime_rate1)
				f3 = open(outfile3, "a")
				f3.write(str(rate1)+"\t")
				f3.close()

				# burst_rate2
				sum_sentbytes_rate2 = 0
				with open(bufferfile) as bufferf:
					line_count = 0
					for line in bufferf:
						line_count += 1
						if line_count <= 1: continue
						array = [x for x in line.split()]
						time = int(array[0])
						sentbytes = float(array[3+5*target_queue_index+2])
						if starttime_rate1 <= time and time <= endtime_rate1:
							sum_sentbytes_rate2 += sentbytes
				if endtime_rate1 == starttime_rate1 and starttime_rate1 > 0:
					rate2 = sum_sentbytes_rate2/float(pullingns)
				else:
					rate2 = sum_sentbytes_rate2/(endtime_rate1-starttime_rate1)
				f4 = open(outfile4, "a")
				f4.write(str(rate2)+"\t")
				f4.close()

			f1 = open(outfile1, "a")
			f1.write("\n")
			f1.close()
			f2 = open(outfile2, "a")
			f2.write("\n")
			f2.close()
			f3 = open(outfile3, "a")
			f3.write("\n")
			f3.close()
			f4 = open(outfile4, "a")
			f4.write("\n")
			f4.close()

	return


def total_num_flow(dir, starttime_list, initialwindow_list, numcontinous):
	outfile1 = dir + "flow_total_num.txt"
	open(outfile1, 'w').close()
	for iw in initialwindow_list:
		for start in starttime_list:
			sum_num_flow = 0
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
				# AnnC: some flows seem to be unlogged
				#assert(len(cwb_fct) == numbursty)
				#assert(len(bonly_fct) == numbursty)
				for cwb_flow_size, cwb_flow_fct in cwb_fct.items():
					for bonly_flow_size, bonly_flow_fct in bonly_fct.items():
						if cwb_flow_size == bonly_flow_size:
							fctslowdown.append(cwb_flow_fct/bonly_flow_fct)
							break
				sum_num_flow += len(fctslowdown)

			# Write out tables
			f1 = open(outfile1, "a")
			f1.write(str(sum_num_flow/10.0)+"\t")
			f1.close()

		f1 = open(outfile1, "a")
		f1.write("\n")
		f1.close()

	return


def summarize_drop(dir, starttime_list, initialwindow_list, numcontinous, numbursty, numqueuesperport, numnodes, numsinks):
	outfile_num_bonly = dir + "drop_num_bonly.txt"
	outfile_num_cwb = dir + "drop_num_cwb.txt"
	outfile_num_diff = dir + "drop_num_diff.txt"
	open(outfile_num_bonly, 'w').close()
	open(outfile_num_cwb, 'w').close()
	open(outfile_num_diff, 'w').close()
	
	target_queue_index = numqueuesperport*(numnodes+numsinks)-1
	for iw in initialwindow_list:
		for start in starttime_list:
			f_num_bonly = open(outfile_num_bonly, "a")
			f_num_bonly.write(str(iw) + "\t" + str(start) + "\t")
			f_num_bonly.close()
			f_num_cwb = open(outfile_num_cwb, "a")
			f_num_cwb.write(str(iw) + "\t" + str(start) + "\t")
			f_num_cwb.close()
			f_num_diff = open(outfile_num_diff, "a")
			f_num_diff.write(str(iw) + "\t" + str(start) + "\t")
			f_num_diff.close()

			numdrop_cwb_allseedslist = list()
			numdrop_bonly_allseedslist = list()
			numdrop_diff_allseedslist = list()
			for seed in range(1,11):
				# Read buffer from file
				cwbfile = dir+"star-burst-tolerance-buffer-"+str(iw)+"-"+str(start)+"-0-"+str(seed)+".data"
				bonlyfile = dir+"star-burst-tolerance-buffer-"+str(iw)+"-"+str(start)+"-1-"+str(seed)+".data"
				sum_numdrop_cwb = 0
				sum_numdrop_bonly = 0
				with open(cwbfile) as cwbf:
					line_count = 0
					for line in cwbf:
						line_count += 1
						if line_count <= 1: continue
						array = [x for x in line.split()]
						droppedbytes = float(array[3+5*target_queue_index+3])
						sum_numdrop_cwb += droppedbytes
				with open(bonlyfile) as bonlyf:
					count = 0
					for line in bonlyf:
						count += 1
						if count <= numcontinous+1: continue
						array = [x for x in line.split()]
						droppedbytes = float(array[3+5*target_queue_index+3])
						sum_numdrop_bonly += droppedbytes
				
				sum_numdrop_diff = sum_numdrop_cwb-sum_numdrop_bonly
				numdrop_cwb_allseedslist.append(sum_numdrop_cwb)
				numdrop_bonly_allseedslist.append(sum_numdrop_bonly)
				numdrop_diff_allseedslist.append(sum_numdrop_diff)

			# Write out tables
			f_num_bonly = open(outfile_num_bonly, "a")
			f_num_bonly.write(str(sum(numdrop_bonly_allseedslist)/len(numdrop_bonly_allseedslist))+"\t")
			f_num_bonly.close()
			f_num_cwb = open(outfile_num_cwb, "a")
			f_num_cwb.write(str(sum(numdrop_cwb_allseedslist)/len(numdrop_cwb_allseedslist))+"\t")
			f_num_cwb.close()
			f_num_diff = open(outfile_num_diff, "a")
			f_num_diff.write(str(sum(numdrop_diff_allseedslist)/len(numdrop_diff_allseedslist))+"\t")
			f_num_diff.close()
		
	return


if __name__ == "__main__":
	dir = "/u/az6922/data/burst-tolerance-aug12/"
	start_list = [0,4500]
	iw_list = [10,20,30]
	numcontinuous = 10
	numbursty = 200
	s3mode = "positive" # "all", "positive"
	# summarize_flow(dir, start_list, iw_list, numcontinuous, numbursty, s3mode)
	# total_num_flow(dir, start_list, iw_list, numcontinuous)
	numqueuesperport = 3
	numnodes = 210
	numsinks = 2
	pullingns = 1e7
	# characterize_burst(dir, start_list, iw_list, numcontinuous, numbursty, numqueuesperport, numnodes, numsinks, pullingns)
	summarize_drop(dir, start_list, iw_list, numcontinuous, numbursty, numqueuesperport, numnodes, numsinks)
