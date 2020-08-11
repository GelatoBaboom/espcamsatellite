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
        cmdRegBtnInit: null,
        cmdRegBtnStop: null,
        messageDeployed: false,
        selectedReg: {
            year: 0,
            month: 0,
            day: 0,
            samples: 0
        },
        stats: {
            maxTemp: 0,
            minTemp: 0,
            maxHum: 0,
            minHum: 0
        },
        init: function (chartElId) {
            var thiscomp = this;
            this.cmdRegBtnInit = $('#cmdRegBtnInit');
            this.cmdRegBtnStop = $('#cmdRegBtnStop');
            this.cmdRegBtnInit.click(function () {
                $.ajax({
                    type: 'GET',
                    dataType: "json",
                    url: '/api/initReg',
                    processData: true,
                    async: true,
                    success: function (resp) {
                        thiscomp.cmdRegBtnInit.fadeOut(1000);
                        thiscomp.dismissMsg();
                        thiscomp.message("Registro iniciado", false);
                    }
                });
            });
            this.cmdRegBtnStop.click(function () {
                $.ajax({
                    type: 'GET',
                    dataType: "json",
                    url: '/api/stopReg',
                    processData: true,
                    async: true,
                    success: function (resp) {
                        thiscomp.cmdRegBtnStop.fadeOut(1000);
                        thiscomp.dismissMsg();
                    }
                });
            });
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
                text: ''
            },
            scales: {
                yAxes: [{
                    type: "linear", // only linear but allow scale type registration. This allows extensions to exist solely for log scale for instance
                    display: true,
                    position: "left",
                    id: "y-axis-1",
                    ticks: {
                        beginAtZero: true,
                        steps: 10,
                        stepValue: 5,
                        max: 70
                    },
					gridLines :{
						lineWidth:0
					}
                },
                {
                    type: "linear", // only linear but allow scale type registration. This allows extensions to exist solely for log scale for instance
                    display: true,
                    position: "right",
                    id: "y-axis-2",
                    ticks: {
                        beginAtZero: true,
                        steps: 10,
                        stepValue: 5,
                        max: 100
                    },
					gridLines :{
						lineWidth:0
					}
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
            },
            {
                label: "Humedad",
                data: [],
                borderColor: "#6BAA3F",
                lineTension: 0.3,
                fill: 'start',
                yAxisID: "y-axis-2",
            }
            ]

        },
        render: function (data) {
            var spData = this.speedData;
            var chrt = this.chartCnf;
            spData.labels = data.labels;
            spData.datasets[0].data = data.tvalues;
            spData.datasets[1].data = data.hvalues;
            this.lineChart = new Chart(this.chartEl, {
                type: 'line',
                data: spData,
                options: chrt
            });
            this.graphRendered = true;
        },
        updateChart: function (data) {
            this.lineChart.data.labels = data.labels;
            this.lineChart.data.datasets[0].data = data.tvalues;
            this.lineChart.data.datasets[1].data = data.hvalues;
            this.lineChart.update();

        },
        renderTempGraph: function () {
            var thiscomp = this;
            if (this.chartEl.length) {
                $.ajax({
                    type: 'GET',
                    dataType: "json",
                    url: '/api/getTempJson',
                    data: {
                        y: thiscomp.selectedReg.year,
                        m: thiscomp.selectedReg.month,
                        d: thiscomp.selectedReg.day,
                        s: thiscomp.selectedReg.samples
                    },
                    processData: true,
                    async: true,
                    success: function (resp) {
                        if (!thiscomp.graphRendered) {
                            thiscomp.render(resp);
                        } else {
                            thiscomp.updateChart(resp);
                        }
                        thiscomp.stats.maxTemp = resp.stats.maxt;
                        thiscomp.stats.minTemp = resp.stats.mint;
                        thiscomp.stats.maxHum = resp.stats.maxh;
                        thiscomp.stats.minHum = resp.stats.minh;
                    }
                });

            }
        },
        downloadRegs: function () {
            var thiscomp = this;
            if (thiscomp.selectedReg.day != 0) {
                $.ajax({
                    type: 'GET',
                    dataType: "json",
                    url: '/api/getTempJson',
                    data: {
                        y: thiscomp.selectedReg.year,
                        m: thiscomp.selectedReg.month,
                        d: thiscomp.selectedReg.day,
                        s: thiscomp.selectedReg.samples
                    },
                    processData: true,
                    async: true,
                    success: function (resp) {
                        var csv = 'hora,temperatura,humedad\r\n';
                        for (var i = 0; i < resp.labels.length; i++) {
                            csv += resp.labels[i] + ',' + resp.tvalues[i] + ',' + resp.hvalues[i] + '\r\n';
                        }
                        thiscomp.makeFile('Registro_' + thiscomp.selectedReg.year + thiscomp.selectedReg.month + thiscomp.selectedReg.day + '.csv', csv);
                    }
                });
            } else {
                thiscomp.loadRegsBase();
                var csv = 'fecha,temperatura,humedad\r\n';
                for (var i = 0; i < thiscomp.registers.length; i++) {
                    if (thiscomp.registers[i].year == thiscomp.selectedReg.year) {
                        for (var j = 0; j < thiscomp.registers[i].months.length; j++) {
                            if (thiscomp.registers[i].months[j].month == thiscomp.selectedReg.month) {
                                for (var k = 0; k < thiscomp.registers[i].months[j].days.length; k++) {
                                    var day = thiscomp.registers[i].months[j].days[k];
                                    $.ajax({
                                        type: 'GET',
                                        dataType: "json",
                                        url: '/api/getTempJson',
                                        data: {
                                            y: thiscomp.selectedReg.year,
                                            m: thiscomp.selectedReg.month,
                                            d: day,
                                            s: thiscomp.selectedReg.samples
                                        },
                                        processData: true,
                                        async: false,
                                        success: function (resp) {
                                            for (var i = 0; i < resp.labels.length; i++) {
                                                csv += (day + '/' + thiscomp.selectedReg.month) + '-' + resp.labels[i] + ',' + resp.tvalues[i] + ',' + resp.hvalues[i] + '\r\n';
                                            }

                                        }
                                    });
                                }
                            }
                        }
                    }
                }
                thiscomp.makeFile('Registro_' + thiscomp.selectedReg.year + thiscomp.selectedReg.month + '.csv', csv);
            }

        },
        makeFile: function (filename, data) {
            var element = document.createElement('a');
            element.setAttribute('href', 'data:text/plain;charset=utf-8,' + encodeURIComponent(data));
            element.setAttribute('download', filename);

            element.style.display = 'none';
            document.body.appendChild(element);

            element.click();

            document.body.removeChild(element);

        }
        ,
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
                                    thiscomp.loadRegsBase();
                                    thiscomp.renderRegList();
                                    thiscomp.renderTempGraph();
                                    thiscomp.setStats('min', 'max');
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
                            thiscomp.loadRegsBase();
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
                    thiscomp.loadRegsBase();
                    thiscomp.selectedReg.year = yvalue.year;
                    thiscomp.selectedReg.month = yvalue.months[yvalue.months.length - 1].month;
                    thiscomp.selectedReg.day = yvalue.months[yvalue.months.length - 1].days[yvalue.months[yvalue.months.length - 1].days.length - 1];
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
                url: '/api/getRegs',
                processData: true,
                async: false,
                success: function (resp) {
                    thiscomp.registers = resp.registers;
                }
            });
        },
        setDateTempNow: function (dateElId, timeElMain, tempElId, mainHumEl, tempDevEl, humInitedEl, calInitedEl) {
            var thiscomp = this;
            $.ajax({
                type: 'GET',
                dataType: "json",
                url: '/api/temp',
                processData: true,
                async: true,
                success: function (resp) {
                    resp.registers;
                    $('#' + dateElId).text((resp.day < 10 ? '0' + resp.day : resp.day) + '/' + (resp.month < 10 ? '0' + resp.month : resp.month));
                    $('#' + timeElMain).text((resp.hour < 10 ? '0' + resp.hour : resp.hour) + ':' + (resp.minute < 10 ? '0' + resp.minute : resp.minute));
                    $('#' + tempElId).text(resp.temp);
                    $('#' + tempDevEl).text(resp.devtemp);
                    $('#' + mainHumEl).text(resp.hum);
                    //dispositivos
                    var hInEL = $('#' + humInitedEl)
                    var cInEL = $('#' + calInitedEl)
                    if (resp.huminited) {
                        hInEL.fadeIn(500);
                    } else {
                        hInEL.fadeOut(500);
                    }
                    if (resp.calinited) {
                        cInEL.fadeIn(500);
                    } else {
                        cInEL.fadeOut(500);
                    }

                    if (resp.rtclostpower) {
                        thiscomp.message("El reloj del sistema esta fuera de hora. registro detenido!", true);
                    } else if (!resp.regenable) {
                        thiscomp.cmdRegBtnInit.fadeIn(500);
                        thiscomp.message("Registro detenido", true);
                    } else {
                        thiscomp.cmdRegBtnStop.fadeIn(500);
                    }
                }
            });
        },
        setStats: function (minTempElId, maxTempElId) {
            var thiscomp = this;
            $('#' + minTempElId).text(thiscomp.stats.minTemp + '/' + thiscomp.stats.minHum + '%');
            $('#' + maxTempElId).text(thiscomp.stats.maxTemp + '/' + thiscomp.stats.maxHum + '%');
        },
        message: function (textMsg, stay) {
            var el = $('#msg');
            el.children().text(textMsg);
            if (!this.messageDeployed) {
                if (stay) {
                    this.messageDeployed = true;
                    el.animate({ top: '+=60' });
                } else {
                    el.animate({ top: '+=60' }).delay(3000).animate({ top: '-=60' });
                }
            }
        },
        dismissMsg: function () {
            if (this.messageDeployed) {
                $('#msg').animate({ top: '-=60' });
                this.messageDeployed = false;
            }
        },
        deleteReg: function () {
            var thiscomp = this;
            if (confirm('queres eliminar este registro?')) {
                $.ajax({
                    type: 'GET',
                    dataType: "json",
                    url: '/api/delReg',
                    processData: true,
                    data: {
                        y: thiscomp.selectedReg.year,
                        m: thiscomp.selectedReg.month,
                        d: thiscomp.selectedReg.day
                    },
                    async: true,
                    success: function (resp) {
                        if (resp.result)
                            thiscomp.message('Registro eliminado', false);
                        thiscomp.loadRegsBase();
                        thiscomp.renderRegList();
                    }
                });
            }
        }
    }
}
$(document).ready(function () {
    var r = new Records('tempChart');
    r.loadRegsBase();
    var yIdx = r.registers.length - 1;
    var mIdx = r.registers[yIdx].months.length - 1;
    var dIdx = r.registers[yIdx].months[mIdx].days.length - 1;

    r.selectedReg.year = r.registers[yIdx].year;
    r.selectedReg.month = r.registers[yIdx].months[mIdx].month;
    r.selectedReg.day = r.registers[yIdx].months[mIdx].days[dIdx];
    r.renderRegList();
    r.renderTempGraph();
    $('#deleteregday').click(function () {
        r.deleteReg();
    });
    $('#downloadregday').click(function () {
        r.downloadRegs();
    });


    var interval = setInterval(function () {
        r.renderTempGraph();
        r.setStats('min', 'max');
    }, 10000);
    r.setDateTempNow('mainDate', 'mainTime', 'mainTemp', 'mainHum', 'devTemp', 'humInited', 'calInited');
    var interval = setInterval(function () {
        r.setDateTempNow('mainDate', 'mainTime', 'mainTemp', 'mainHum', 'devTemp', 'humInited', 'calInited');
    }, 5000);
});
