#ifndef CADMIUM_EXAMPLE_GPT_TRANSDUCER_HPP_
#define CADMIUM_EXAMPLE_GPT_TRANSDUCER_HPP_

#include <cadmium/modeling/devs/atomic.hpp>
#include <iostream>
#include "job.hpp"

namespace cadmium::example::gpt {
	//! Class for representing the Transducer DEVS model state.
	struct TransducerState {
		double clock;        //!< Current simulation time.
		double sigma;        //!< Time to wait before triggering the next internal transition function.
		double totalTA;      //!< Accumulated processing time for all the Job objects processed by the Processor.
		int nJobsGenerated;  //!< number of Job objects generated by the Generator.
		int nJobsProcessed;  //!< number of Job objects processed by the Processor.

		/**
		 * Transducer model state constructor function.
		 * @param obsTime total observation time before asking the Generator to stop creating new Job objects.
		 */
		explicit TransducerState(double obsTime) : clock(), sigma(obsTime), totalTA(), nJobsGenerated(), nJobsProcessed() {}
	};

	/**
	 * Insertion operator for TransducerState objects.
	 * @param out output stream.
	 * @param s state to be represented in the output stream.
	 * @return output stream with the state already inserted.
	 */
	std::ostream& operator<<(std::ostream &out, const TransducerState& s) {
		out << "{" << s.totalTA << "," << s.nJobsGenerated << "," << s.nJobsProcessed << "}";
		return out;
	}

	//! Atomic DEVS model of a Job transducer.
	class Transducer : public Atomic<TransducerState> {
	 public:
		BigPort<Job> inGenerated;  //!< Input Port for receiving new Job objects.
		BigPort<Job> inProcessed;  //!< Input Port for receiving processed Job objects.
		Port<bool> outStop;     //!< Output Port for asking Generator to stop generating Job objects.

		/**
		 * Constructor function for Transducer atomic DEVS models.
		 * @param id ID of the new model.
		 * @param obsTime Time to wait before asking the Generator to stop sending new Job objects to Processor.
		 */
		Transducer(const std::string& id, double obsTime): Atomic<TransducerState>(id, TransducerState(obsTime)) {
			inGenerated = addInBigPort<Job>("inGenerated");
			inProcessed = addInBigPort<Job>("inProcessed");
			outStop = addOutPort<bool>("outStop");
		}

		/**
		 * It updates TransducerState::clock and TransducerState::sigma.
		 * As the internal transition is only triggered at the end, TransducerState::sigma is set to infinity.
		 * It also prints some simulation statistics.
		 * @param s reference to the current state of the model.
		 */
		void internalTransition(TransducerState& s) const override {
			s.clock += s.sigma;
			s.sigma = std::numeric_limits<double>::infinity();

			std::cout << "End time: " << s.clock << std::endl;
			std::cout << "Jobs generated: " << s.nJobsGenerated << std::endl;
			std::cout << "Jobs processed: " << s.nJobsProcessed << std::endl;
			if (s.nJobsProcessed > 0) {
				std::cout << "Average TA: " << s.totalTA / (double) s.nJobsProcessed << std::endl;
			}
			if (s.clock > 0) {
				std::cout << "Throughput: " << (double) s.nJobsProcessed /  s.clock << std::endl;
			}
		}

		/**
		 * It updates TransducerState::clock and TransducerState::sigma.
		 * It also keeps track of the Job objects generated by the Generator and processed by the Processor.
		 * @param s reference to the current model state.
		 * @param e time elapsed since the last state transition function was triggered.
		 * @param x reference to the model input port set.
		 */
		void externalTransition(TransducerState& s, double e) const override {
			s.clock += e;
			s.sigma -= e;
			for (auto& job: inGenerated->getBag()) {  // TODO discuss this...
				s.nJobsGenerated += 1;
				std::cout << "Job " << job->id << " generated at t = " << s.clock << std::endl;
			}
			for (auto& job: inProcessed->getBag()) {  // TODO ... vs this
				s.nJobsProcessed += 1;
				s.totalTA += job->timeProcessed - job->timeGenerated;
				std::cout << "Job " << job->id << " processed at t = " << s.clock << std::endl;
			}
		}

		/**
		 * It sends a message to the Generator to stop generating new Job objects.
		 * @param s reference to the current model state.
		 * @param y reference to the atomic model output port set.
		 */
		void output(const TransducerState& s) const override {
			outStop->addMessage(true);
		}

		/**
		 * It returns the value of TransducerState::sigma.
		 * @param s reference to the current model state.
		 * @return the sigma value.
		 */
		[[nodiscard]] double timeAdvance(const TransducerState& s) const override {
			return s.sigma;
		}
	};
}  //namespace cadmium::example::gpt

#endif //CADMIUM_EXAMPLE_GPT_TRANSDUCER_HPP_
