def summarize_flow(dir, starttime_list, initialwindow_list, numcontinous, numbursty, s3mode="all"):
	outfile1 = dir + "average_all.txt"
	# outfile2 = dir + "average_slow.txt"
	outfile3 = dir + "num_slow.txt"
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
			num_trial_num_slow = 0
			if s3mode == "positive":
				for slowdownlist in fctslowdown_allseedslist:
					count_sd = 0
					for sd in slowdownlist:
						if sd > 1:
							count_sd += 1
					if count_sd > 0:
						count_num_slow += count_sd
						num_trial_num_slow += 1
				f3 = open(outfile3, "a")
				f3.write(str(count_num_slow/float(num_trial_num_slow))+"\t")
			elif s3mode == "all":
				for slowdownlist in fctslowdown_allseedslist:
					count_sd = 0
					for sd in slowdownlist:
						if sd > 1:
							count_sd += 1
					count_num_slow += count_sd
				f3 = open(outfile3, "a")
				f3.write(str(count_num_slow/float(len(slowdownlist)))+"\t")
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
def characterize_burst(dir, starttime_list, initialwindow_list, numcontinous, numbursty, s3mode="all"):
	outfile1 = dir + "size1.txt"
	outfile2 = dir + "size2.txt"
	outfile3 = dir + "rate1.txt"
	outfile4 = dir + "rate2.txt"
	open(outfile1, 'w').close()
	open(outfile2, 'w').close()
	open(outfile3, 'w').close()
	open(outfile4, 'w').close()

	for iw in initialwindow_list:
		for start in starttime_list:
			for seed in range(1,11):
				# Read from file
				flowfile = dir+"star-burst-tolerance-flow-"+str(iw)+"-"+str(start)+"-1-"+str(seed)+".data"
				bufferfile = dir+"star-burst-tolerance-buffer-"+str(iw)+"-"+str(start)+"-1-"+str(seed)+".data"
				paramfile = dir+"star-burst-tolerance-parameters-"+str(iw)+"-"+str(start)+"-1-"+str(seed)+".data"
				
				with open(paramfile) as paramf:
					count = 0
					for line in paramf:
						count += 1
						if count <= 1: continue
						array = [x for x in line.split(',')]
						flowsizephrase = array[1]
						phrasearray = [x for x in flowsizephrase.split(',')]
						flowsize = float(phrasearray[1])
						print(flowsize)
				
				
				

			# Write out tables
			# 1. Average FCT slowdown across all flows
			# sum_fct_all = 0
			# count_fct_all = 0
			# for slowdownlist in fctslowdown_allseedslist:
			# 	avg_sd = sum(slowdownlist) / len(slowdownlist)
			# 	sum_fct_all += avg_sd
			# 	count_fct_all += 1
			# f1 = open(outfile1, "a")
			# f1.write(str(sum_fct_all/count_fct_all)+"\t")
			# f1.close()

			# # 2. Average FCT slowdown across flows that have been slowed down
			# # sum_fct_slow = 0
			# # count_fct_slow = 0
			# # for slowdownlist in fctslowdown_allseedslist:
			# # 	sum_sd = 0
			# # 	count_sd = 0
			# # 	for sd in slowdownlist:
			# # 		if sd > 1:
			# # 			sum_sd += sd
			# # 			count_sd += 1
			# # 	if count_sd > 0:
			# # 		avg_sd = sum_sd / count_sd
			# # 	else:
			# # 		avg_sd = 0
			# # 	if avg_sd > 0:
			# # 		sum_fct_slow += avg_sd
			# # 		count_fct_slow += 1
			# # f2 = open(outfile2, "a")
			# # f2.write(str(sum_fct_slow/count_fct_slow)+"\t")
			# # f2.close()

			# # 3. Number of flows that have been slowed down
			# count_num_slow = 0
			# num_trial_num_slow = 0
			# if s3mode == "positive":
			# 	for slowdownlist in fctslowdown_allseedslist:
			# 		count_sd = 0
			# 		for sd in slowdownlist:
			# 			if sd > 1:
			# 				count_sd += 1
			# 		if count_sd > 0:
			# 			count_num_slow += count_sd
			# 			num_trial_num_slow += 1
			# 	f3 = open(outfile3, "a")
			# 	f3.write(str(count_num_slow/float(num_trial_num_slow))+"\t")
			# elif s3mode == "all":
			# 	for slowdownlist in fctslowdown_allseedslist:
			# 		count_sd = 0
			# 		for sd in slowdownlist:
			# 			if sd > 1:
			# 				count_sd += 1
			# 		count_num_slow += count_sd
			# 	f3 = open(outfile3, "a")
			# 	f3.write(str(count_num_slow/float(len(slowdownlist)))+"\t")
			# f3.close()

		f1 = open(outfile1, "a")
		f1.write("\n")
		f1.close()
		f2 = open(outfile2, "a")
		f2.write("\n")
		f2.close()
		f3 = open(outfile3, "a")
		f3.write("\n")
		f3.close()

	return


if __name__ == "__main__":
	dir = "/u/az6922/data/burst-tolerance-aug5/"
	start_list = [0,4500]
	# iw_list = [5,50,100,500,1000]
	iw_list = [5]
	numcontinuous = 10
	numbursty = 10
	s3mode = "positive" # "all", "positive"
	# summarize_flow(dir, start_list, iw_list, numcontinuous, numbursty, s3mode)
	characterize_burst(dir, start_list, iw_list, numcontinuous, numbursty, s3mode)
