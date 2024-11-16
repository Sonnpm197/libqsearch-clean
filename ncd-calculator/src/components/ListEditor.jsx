import React, {useState} from 'react';
import { WithContext as ReactTags } from "react-tag-input";
import "../css/InputTags.css";
import ANIMALS from "../search-suggestion/animals.js";

const ListEditor = ({performSearch}) => {

    const [selectedItems, setSelectedItems] = useState([]);
    const [projections, setProjections] = useState({
        Accession: true,
        ScientificName: false,
        CommonName: true,
        FileName: false,
    });
    const [apiKey, setApiKey] = useState('');

    const onPerformSearch = () => {
        performSearch(selectedItems, projections, apiKey);
    }

    const isDisabledByApiKey = apiKey === null || apiKey.trim() === '';

    const handleDelete = (index) => {
        setSelectedItems(selectedItems.filter((_, i) => i !== index));
    };

    const onTagUpdate = (index, newTag) => {
        const updatedItems = [...selectedItems];
        updatedItems.splice(index, 1, newTag);
        setSelectedItems(updatedItems);
    };

    const handleAddition = (item) => {
        setSelectedItems((prevItems) => {
            return [...prevItems, item];
        });
    };

    const suggestions = ANIMALS.map((animal) => {
        return {
            id: animal,
            text: animal,
            className: "",
        };
    });

    return (
        <>
            <div className={`${isDisabledByApiKey ? 'pointer-events-none' : 'pointer-events-auto'}`}>
                <ReactTags
                    tags={selectedItems}
                    inputFieldPosition="top"
                    suggestions={suggestions}
                    handleDelete={handleDelete}
                    handleAddition={handleAddition}
                    onTagUpdate={onTagUpdate}
                    editable
                    maxTags={50}
                    autoFocus={false}
                    placeholder="Input search terms and press Enter (at least 4)"
                    classNames={{
                        tagInputField: `${isDisabledByApiKey ? 'bg-gray-300' : 'bg-white'} w-full text-lg mb-4 p-4 text-black border border-gray-300 rounded-lg`,
                        tag: `${isDisabledByApiKey ? 'bg-gray-300' : 'bg-[#3182ce]'} border border-gray-300 text-white text-lg px-2 py-1 mx-1 font-semibold rounded-md flex items-center`
                    }}
                />
            </div>

            <input type="text"
                   className="block w-full text-lg p-4 mb-6 text-black bg-white border border-gray-300 rounded-lg"
                   value={apiKey}
                   autoFocus={true}
                   placeholder="Input API Key to enable actions"
                   onChange={(event) => setApiKey(event.target.value)}/>

            <div
                className={`flex items-center justify-between mt-6 mb-6 ${isDisabledByApiKey ? 'pointer-events-none' : 'pointer-events-auto'}`}>
                <div style={{display: 'flex', gap: '12px', flexWrap: 'wrap'}}>
                    {Object.entries(projections).map(([key, value]) => (
                        <label
                            className={`${isDisabledByApiKey ? 'bg-gray-300' : (value ? 'bg-blue-200' : 'bg-white')}
                                flex items-center gap-2 py-2 px-4 rounded cursor-pointer border border-gray-200`}
                            key={key}>
                            <input
                                type="checkbox"
                                checked={value}
                                onChange={() => setProjections(prev => ({
                                    ...prev,
                                    [key]: key !== "Accession" ? !prev[key] : prev[key]
                                }))}
                            />
                            <span style={{color: '#2d3748'}}>{key.replace(/([A-Z])/g, ' $1').trim()}</span>
                        </label>
                    ))}
                </div>

                <button
                    onClick={onPerformSearch}
                    className={`${isDisabledByApiKey || selectedItems.length < 4 ? 'bg-gray-300 text-gray-800' : 'bg-blue-600 text-white hover:bg-blue-900'}
                        "py-3 px-6 text-base rounded-lg cursor-pointer border-none shadow-md transition-colors duration-200"`}>
                    Search
                </button>
            </div>
        </>
    );
};

export default ListEditor;