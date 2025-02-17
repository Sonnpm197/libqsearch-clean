import React, {useState, useEffect} from 'react';
import {Search, ChevronRight, X} from 'lucide-react';

const ListEditor = ({performSearch}) => {

    const [searchTerm, setSearchTerm] = useState('');
    const [searchResults, setSearchResults] = useState([]);
    const [selectedItems, setSelectedItems] = useState([]);
    const [projections, setProjections] = useState({
        Accession: true,
        ScientificName: false,
        CommonName: true,
        FileName: false,
    });


    const onInputChange = (term) => {
        setSearchTerm(term);
        setSearchResults([]);
    }

    const onPerformSearch = () => {
        if (selectedItems.length !== 0) {
            performSearch(selectedItems, projections);
        }
    }


    const handleKeyDown = async (e) => {
        if (e.key === "Enter") {
            if (searchTerm && searchTerm.trim() !== '')
                setSearchResults([...searchResults, searchTerm]);
        }
    };


    return (
        <div style={{padding: '20px', width: '100%'}}>
            {/* Main Content - Side by Side Layout */}
            <div style={{display: 'flex', gap: '24px', flex: 1, width: '60vw'}}>
                <div style={{
                    width: '50%',
                    height: '500px',
                    overflowY: 'auto',
                    border: '1px solid #ccc',
                    padding: '24px',
                    boxSizing: 'border-box',
                    borderRadius: '12px',
                    backgroundColor: 'white'
                }}>
                    <h3 style={{
                        fontSize: '1.2rem',
                        fontWeight: 'bold',
                        marginBottom: '20px',
                        color: '#1a365d'
                    }}>Search
                        Results</h3>
                    <div style={{position: 'relative', marginBottom: '20px'}}>
                        <div
                            style={{position: 'absolute', left: '16px', top: '50%', transform: 'translateY(-50%)'}}>
                            <Search size={20} color="#4a5568"/>
                        </div>
                        <input
                            type="text"
                            value={searchTerm}
                            onKeyDown={handleKeyDown}
                            onChange={(e) => onInputChange(e.target.value)}
                            placeholder="Search animals..."
                            style={{
                                width: '100%',
                                padding: '12px 12px 12px 48px',
                                border: '2px solid #e2e8f0',
                                borderRadius: '8px',
                                fontSize: '1rem',
                                color: 'black', // Set text color to black
                                backgroundColor: '#f8fafc',
                                outline: 'none',
                                transition: 'border-color 0.2s',
                            }}
                            onFocus={(e) => e.target.style.borderColor = '#3182ce'}
                            onBlur={(e) => e.target.style.borderColor = '#e2e8f0'}
                        />
                    </div>

                    <div style={{
                        display: 'flex',
                        flexDirection: 'column',
                        gap: '10px',
                        maxHeight: '500px',
                        overflowY: 'auto',
                        padding: '4px'
                    }}>
                        {searchResults.map((item, index) => (
                            <div
                                key={index}
                                onClick={() => setSelectedItems(prev => [...prev, item])}
                                style={{
                                    display: 'flex',
                                    justifyContent: 'space-between',
                                    alignItems: 'center',
                                    padding: '16px',
                                    backgroundColor: '#f8fafc',
                                    border: '1px solid #e2e8f0',
                                    borderRadius: '8px',
                                    cursor: 'pointer',
                                    transition: 'all 0.2s'
                                }}
                            >
                                <div>
                                    {<div style={{color: '#2d3748'}}>{searchTerm}</div>}
                                </div>
                                <ChevronRight size={20} color="#a0aec0"/>
                            </div>
                        ))}
                    </div>
                </div>

                {/* Selected Items Box */}


                <div style={{
                    width: '50%',
                    height: '500px',
                    overflowY: 'auto',
                    border: '1px solid #ccc',
                    padding: '24px',
                    boxSizing: 'border-box',
                    borderRadius: '12px',
                    backgroundColor: 'white'
                }}>

                    <h3 style={{
                        fontSize: '1.2rem',
                        fontWeight: 'bold',
                        marginBottom: '20px',
                        color: '#1a365d'
                    }}>Selected Items</h3>
                    <div style={{
                        display: 'flex',
                        flexDirection: 'column',
                        gap: '10px',
                        maxHeight: '500px',
                        overflowY: 'auto',
                        padding: '4px'
                    }}>
                        {selectedItems.map((item, index) => (
                            <div
                                key={index}
                                style={{
                                    display: 'flex',
                                    justifyContent: 'space-between',
                                    alignItems: 'center',
                                    padding: '16px',
                                    backgroundColor: '#f8fafc',
                                    border: '1px solid #e2e8f0',
                                    borderRadius: '8px',
                                }}
                            >
                                <div>
                                    <div style={{color: '#718096', fontSize: '0.9rem'}}>{selectedItems[index]}</div>
                                </div>
                                <X size={20} color="#a0aec0" style={{cursor: 'pointer'}} onClick={() =>
                                    setSelectedItems(prev => prev.filter((_, i) => i !== index))
                                }/>
                            </div>
                        ))}
                    </div>
                </div>
            </div>


            <div style={{
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'space-between',
                marginBottom: '24px',
                marginTop: '24px'
            }}>
                <div style={{display: 'flex', gap: '12px', flexWrap: 'wrap'}}>
                    {Object.entries(projections).map(([key, value]) => (
                        <label key={key} style={{
                            display: 'flex',
                            alignItems: 'center',
                            gap: '8px',
                            padding: '8px 16px',
                            backgroundColor: value ? '#e3f2fd' : 'white',
                            borderRadius: '6px',
                            cursor: 'pointer',
                            border: '1px solid #e2e8f0'
                        }}>
                            <input
                                type="checkbox"
                                checked={value}
                                onChange={() => setProjections(prev => ({
                                    ...prev,
                                    [key]: key !== "Accession" ? !prev[key] : prev[key]
                                }))}
                                style={{cursor: 'pointer'}}
                            />
                            <span style={{color: '#2d3748'}}>{key.replace(/([A-Z])/g, ' $1').trim()}</span>
                        </label>
                    ))}
                </div>


                <button
                    onClick={onPerformSearch}
                    style={{
                        padding: '12px 24px',
                        backgroundColor: '#3182ce',
                        color: 'white',
                        fontSize: '1rem',
                        borderRadius: '8px',
                        cursor: 'pointer',
                        border: 'none',
                        boxShadow: '0 4px 6px rgba(0, 0, 0, 0.1)',
                        transition: 'background-color 0.2s'
                    }}
                    onMouseEnter={(e) => e.target.style.backgroundColor = '#2b6cb0'}
                    onMouseLeave={(e) => e.target.style.backgroundColor = '#3182ce'}
                >
                    Search
                </button>
            </div>
        </div>
    );
};

export default ListEditor;