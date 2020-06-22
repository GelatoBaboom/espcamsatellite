function Records(chartElId) {
	this.me = new Records_CLASS();
	this.me.init(chartElId);
	return this.me;
}
Records.prototype.action = function () {
	//return this.click;
}
function Records_CLASS() {
	return {
		chartEl: null,
		graphRendered: false,
		lineChart: null,
		registers: null,
		selectedReg: {
			year: 0,
			month: 0,
			day: 0,
			samples: 0
		},
		stats: {
			maxTemp: 0,
			minTemp: 0
		},
		init: function (chartElId) {
			var thiscomp = this;
			this.chartEl = $('#' + chartElId);
			var sampleInp = $('#' + chartElId + 'Samples');
			sampleInp.change(function () {
				thiscomp.selectedReg.samples = sampleInp.val();
				$('#' + chartElId + 'SamplesLabel').text(sampleInp.attr('max') - (sampleInp.val() - 1));
				thiscomp.renderTempGraph();
			});
		},
		chartCnf: {
			responsive: true,
			aspectRatio: 1,
			hoverMode: 'index',
			stacked: false,
			title: {
				display: true,
				text: 'Grafico de temperatura'
			},
			scales: {
				yAxes: [{
						type: "linear", // only linear but allow scale type registration. This allows extensions to exist solely for log scale for instance
						display: true,
						position: "left",
						id: "y-axis-1",
					}
				],
			}
		},
		speedData: {
			labels: [],
			datasets: [{
					label: "Temperatura",
					data: [],
					borderColor: "#0368AE",
					lineTension: 0.3,
					fill: 'start',
					yAxisID: "y-axis-1",
				}
			]

		},
		render: function (data) {
			var spData = this.speedData;
			var chrt = this.chartCnf;
			spData.labels = data.labels;
			spData.datasets[0].data = data.values;
			this.lineChart = new Chart(this.chartEl, {
					type: 'line',
					data: spData,
					options: chrt
				});
			this.graphRendered = true;
		},
		updateChart: function (data) {
			this.lineChart.data.labels = data.labels;
			this.lineChart.data.datasets[0].data = data.values;
			this.lineChart.update();

		},
		renderTempGraph: function () {
			var thiscomp = this;
			if (this.chartEl.length) {
				$.ajax({
					type: 'GET',
					dataType: "json",
					url: '/getTempJson',
					data: {
						y: thiscomp.selectedReg.year,
						m: thiscomp.selectedReg.month,
						d: thiscomp.selectedReg.day,
						s: thiscomp.selectedReg.samples
					},
					processData: true,
					async: false,
					success: function (resp) {
						if (!thiscomp.graphRendered) {
							thiscomp.render(resp);
						} else {
							thiscomp.updateChart(resp);
						}
						thiscomp.stats.maxTemp = resp.stats.max;
						thiscomp.stats.minTemp = resp.stats.min;
					}
				});

			}
		},
		renderRegList: function () {
			var thiscomp = this;
			var yCont = $('#yCont');
			var mCont = $('#mCont');
			var dCont = $('#dCont');
			yCont.empty();
			mCont.empty();
			dCont.empty();
			$.each(thiscomp.registers, function (idx, yvalue) {
				var yEl = null;
				if (thiscomp.selectedReg.year == yvalue.year) {
					yEl = $('<div/>', {
							'class': 'datedv datedvSel',
							'text': yvalue.year
						});
					$.each(yvalue.months, function (idx, mvalue) {
						var mEl = null;
						if (thiscomp.selectedReg.month == mvalue.month) {
							mEl = $('<div/>', {
									'class': 'datedv datedvSel',
									'text': mvalue.month
								});
							$.each(mvalue.days, function (idx, dvalue) {
								var dEl = null;
								if (thiscomp.selectedReg.day == dvalue) {
									dEl = $('<div/>', {
											'class': 'datedv datedvSel',
											'text': dvalue
										});
								} else {
									dEl = $('<div/>', {
											'class': 'datedv',
											'text': dvalue
										});
								}
								dEl.click(function () {
									thiscomp.selectedReg.year = yvalue.year;
									thiscomp.selectedReg.month = mvalue.month;
									thiscomp.selectedReg.day = dvalue;
									thiscomp.renderRegList();
									thiscomp.renderTempGraph();
								});
								dCont.append(dEl);
							});
						} else {
							mEl = $('<div/>', {
									'class': 'datedv',
									'text': mvalue.month
								});
						}
						mEl.click(function () {
							thiscomp.selectedReg.month = mvalue.month;
							thiscomp.selectedReg.day = 0;
							thiscomp.renderRegList();
							thiscomp.renderTempGraph();
						});
						mCont.append(mEl);
					});
				} else {
					yEl = $('<div/>', {
							'class': 'datedv',
							'text': yvalue.year
						});
				}
				yEl.click(function () {
					thiscomp.selectedReg.year = yvalue.year;
					thiscomp.selectedReg.month = yvalue.months[0].month;
					thiscomp.selectedReg.day = yvalue.months[0].days[0];
					thiscomp.renderRegList();
				});
				yCont.append(yEl);
			});

		},
		loadRegsBase: function () {
			var thiscomp = this;
			$.ajax({
				type: 'GET',
				dataType: "json",
				url: '/getRegs',
				processData: true,
				async: false,
				success: function (resp) {
					thiscomp.registers = resp.registers;
				}
			});
		},
		setDateTempNow: function (dateElId, timeElMain, tempElId) {
			var thiscomp = this;
			$.ajax({
				type: 'GET',
				dataType: "json",
				url: '/temp',
				processData: true,
				async: false,
				success: function (resp) {
					resp.registers;
					$('#' + dateElId).text((resp.day < 10 ? '0' + resp.day : resp.day) + '/' + (resp.month < 10 ? '0' + resp.month : resp.month));
					$('#' + timeElMain).text((resp.hour < 10 ? '0' + resp.hour : resp.hour) + ':' + (resp.minute < 10 ? '0' + resp.minute : resp.minute));
					$('#' + tempElId).text(resp.temp);
				}
			});
		},
		setStats: function (minTempElId, maxTempElId) {
			var thiscomp = this;
			$('#' + minTempElId).text(thiscomp.stats.minTemp);
			$('#' + maxTempElId).text(thiscomp.stats.maxTemp);
		}

	}
}
$(document).ready(function () {
	var r = new Records('tempChart');
	r.loadRegsBase();
	console.log(r);
	r.selectedReg.year = r.registers[r.registers.length - 1].year;
	r.selectedReg.month = r.registers[r.registers.length - 1].months[0].month;
	r.selectedReg.day = r.registers[r.registers.length - 1].months[0].days[r.registers[r.registers.length - 1].months[0].days.length - 1];
	r.renderRegList();
	r.renderTempGraph();

	var interval = setInterval(function () {
			r.renderTempGraph();
			r.setStats('minT','maxT');
		}, 10000);
	r.setDateTempNow('mainDate', 'mainTime', 'mainTemp');
	var interval = setInterval(function () {
			r.setDateTempNow('mainDate', 'mainTime', 'mainTemp');
		}, 30000);
});
